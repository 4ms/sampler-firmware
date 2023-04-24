#include "audio_stream.hh"
#include "bl_utils.hh"
#include "bootloader/animation.hh"
#include "bootloader/buttons.hh"
#include "bootloader/gate_input.hh"
#include "bootloader/leds.hh"
#include "conf/bootloader_settings.hh"
#include "conf/flash_layout.hh"
#include "drivers/stm32xx.h"
#include "flash.hh"
#include "system.hh"
#include "util/zip.hh"

#define USING_FSK
// #define USING_QPSK

#ifdef USING_QPSK
#include "stm_audio_bootloader/qpsk/demodulator.h"
#include "stm_audio_bootloader/qpsk/packet_decoder.h"
#else
#include "stm_audio_bootloader/fsk/demodulator.h"
#include "stm_audio_bootloader/fsk/packet_decoder.h"
#endif

namespace SamplerKit::Bootloader
{

struct AudioBootloader {
#ifdef USING_QPSK
	constexpr float kModulationRate = 6000.0;
	constexpr float kBitRate = 12000.0;
	constexpr float kSampleRate = 48000.0;
#else
	static constexpr uint32_t kSampleRate = BootloaderConf::SampleRate;		 //-s
	static constexpr uint32_t kPausePeriod = BootloaderConf::Encoding.blank; //-b
	static constexpr uint32_t kOnePeriod = BootloaderConf::Encoding.one;	 //-n
	static constexpr uint32_t kZeroPeriod = BootloaderConf::Encoding.zero;	 //-z
#endif
	static constexpr uint32_t kStartReceiveAddress = BootloaderReceiveAddr;
	static constexpr uint32_t kBlkSize = BootloaderConf::ReceiveSectorSize;					   // Flash page size, -g
	static constexpr uint16_t kPacketsPerBlock = kBlkSize / stm_audio_bootloader::kPacketSize; // kPacketSize=256

	uint8_t recv_buffer[kBlkSize];

	stm_audio_bootloader::PacketDecoder decoder;
	stm_audio_bootloader::Demodulator demodulator;

	uint16_t packet_index;
	uint16_t discard_samples = 8000;
	uint32_t current_flash_address;

	enum UiState { UI_STATE_WAITING, UI_STATE_RECEIVING, UI_STATE_ERROR, UI_STATE_WRITING, UI_STATE_DONE };
	UiState ui_state;

	AudioBootloader() {
		init_leds();
		init_buttons();

		animate(ANI_RESET);
	}

	bool check_enter_bootloader() {
		HAL_Delay(300);

		uint32_t dly = 32000;
		uint32_t button_debounce = 0;
		while (dly--) {
			if (button_pushed(Button::Rev) && button_pushed(Button::Bank))
				button_debounce++;
			else
				button_debounce = 0;
		}

		HAL_Delay(100);

		bool do_bootloader = (button_debounce > 15000);
		return do_bootloader;
	}

	void run() {
		init_reception();

		AudioStream audio_stream(
			[this](const AudioStreamConf::AudioInBlock &inblock, AudioStreamConf::AudioOutBlock &outblock) {
				for (auto [in, out] : zip(inblock, outblock)) {
					bool sample = std::abs(in.sign_extend_chan(1)) > (0x007FFFFFU / 32U); // 10Vpeak => 300mV threshold

					if (!discard_samples) {
						demodulator.PushSample(sample ? 1 : 0);
					} else {
						--discard_samples;
					}

					out.chan[0] = this->ui_state != UI_STATE_ERROR ? in.chan[1] : 0;
					out.chan[1] = sample ? 0xC00000 : 0x400000;
				}
			});
		audio_stream.start();

		uint32_t button1_exit_armed = 0;
		uint32_t cycle_but_armed = 0;

		while (button_pushed(Button::Rev) || button_pushed(Button::Bank))
			;

		HAL_Delay(300);

		uint8_t exit_updater = false;
		while (!exit_updater) {
			stm_audio_bootloader::PacketDecoderState state;
			uint32_t symbols_processed = 0;
			uint8_t symbol;
			bool rcv_err = false;

			while (demodulator.available() && !rcv_err && !exit_updater) {
				symbol = demodulator.NextSymbol();
				state = decoder.ProcessSymbol(symbol);
				symbols_processed++;

				switch (state) {
					case stm_audio_bootloader::PACKET_DECODER_STATE_SYNCING:
						animate(ANI_SYNC);
						break;

					case stm_audio_bootloader::PACKET_DECODER_STATE_OK:
						ui_state = UI_STATE_RECEIVING;
						memcpy(recv_buffer + (packet_index % kPacketsPerBlock) * stm_audio_bootloader::kPacketSize,
							   decoder.packet_data(),
							   stm_audio_bootloader::kPacketSize);
						++packet_index;
						if ((packet_index % kPacketsPerBlock) == 0) {
							ui_state = UI_STATE_WRITING;
							bool write_ok = write_buffer();
							if (!write_ok) {
								ui_state = UI_STATE_ERROR;
								rcv_err = true;
							}
							new_block();
						} else {
							new_packet();
						}
						break;

					case stm_audio_bootloader::PACKET_DECODER_STATE_ERROR_SYNC:
						rcv_err = true;
						break;

					case stm_audio_bootloader::PACKET_DECODER_STATE_ERROR_CRC:
						rcv_err = true;
						break;

					case stm_audio_bootloader::PACKET_DECODER_STATE_END_OF_TRANSMISSION:
						if (current_flash_address == kStartReceiveAddress) {
							if (!write_buffer()) {
								ui_state = UI_STATE_ERROR;
								rcv_err = true;
								new_block();
								break;
							}
						}
						exit_updater = true;
						ui_state = UI_STATE_DONE;
						animate_until_button_pushed(ANI_SUCCESS, Button::Play);
						animate(ANI_RESET);
						HAL_Delay(100);
						break;

					default:
						break;
				}
			}
			if (rcv_err) {
				ui_state = UI_STATE_ERROR;
				animate_until_button_pushed(ANI_FAIL_ERR, Button::Play);
				animate(ANI_RESET);
				HAL_Delay(100);
				init_reception();
				exit_updater = false;
			}

			if (button_pushed(Button::Rev)) {
				if (cycle_but_armed) {
					HAL_Delay(100);
					init_reception();
				}
				cycle_but_armed = 0;
			} else
				cycle_but_armed = 1;

			if (button_pushed(Button::Play)) {
				if (button1_exit_armed) {
					if (ui_state == UI_STATE_WAITING) {
						exit_updater = true;
					}
				}
				button1_exit_armed = 0;
			} else
				button1_exit_armed = 1;
		}
		ui_state = UI_STATE_DONE;
		while (button_pushed(Button::Play) || button_pushed(Button::Rev)) {
			;
		}
	}

	void init_reception() {
#ifdef USING_QPSK
		// QPSK
		decoder.Init((uint16_t)20000);
		demodulator.Init(
			kModulationRate / kSampleRate * 4294967296.0f, kSampleRate / kModulationRate, 2.f * kSampleRate / kBitRate);
		demodulator.SyncCarrier(true);
		decoder.Reset();
#else
		// FSK
		decoder.Init();
		decoder.Reset();
		demodulator.Init(kPausePeriod, kOnePeriod, kZeroPeriod); // pause_thresh = 24. one_thresh = 6.
		demodulator.Sync();
#endif

		current_flash_address = kStartReceiveAddress;
		packet_index = 0;
		ui_state = UI_STATE_WAITING;
	}

	bool write_buffer() {
		if ((current_flash_address + kBlkSize) <= get_sector_addr(NumFlashSectors)) {
			flash_write_page(recv_buffer, current_flash_address, kBlkSize);
			current_flash_address += kBlkSize;
			return true;
		} else {
			return false;
		}
	}

	void update_LEDs() {
		if (ui_state == UI_STATE_RECEIVING)
			animate(ANI_RECEIVING);

		else if (ui_state == UI_STATE_WRITING)
			animate(ANI_WRITING);

		else if (ui_state == UI_STATE_WAITING)
			animate(ANI_WAITING);

		else // if (ui_state == UI_STATE_DONE)
		{}
	}

	void new_block() {
		decoder.Reset();
#ifdef USING_FSK
		demodulator.Sync(); // FSK
#else
		demodulator.SyncCarrier(false); // QPSK
#endif
	}

	void new_packet() {
#ifdef USING_FSK
		decoder.Reset(); // FSK
#else
		demodulator.SyncDecision();		// QPSK
#endif
	}

	void animate_until_button_pushed(Animations animation_type, Button button) {
		animate(ANI_RESET);

		while (!button_pushed(button)) {
			HAL_Delay(1);
			animate(animation_type);
		}
		while (button_pushed(button)) {
			HAL_Delay(1);
		}
	}
};

} // namespace SamplerKit::Bootloader

void main() {

	HAL_Init();
	SamplerKit::Bootloader::System system_init;
	SamplerKit::Bootloader::AudioBootloader bootloader;

	mdrivlib::Timekeeper update_task{{
										 .TIMx = TIM7,
										 .period_ns = 1'000'000'000 / 1'000, // 1'000Hz = 1ms
										 .priority1 = 1,
										 .priority2 = 1,
									 },
									 [&] { bootloader.update_LEDs(); }};
	update_task.start();

	// if (bootloader.check_enter_bootloader())
	bootloader.run();

	mdrivlib::System::reset_buses();
	mdrivlib::System::reset_RCC();
	mdrivlib::System::jump_to(AppStartAddr);
	while (true) {
		__NOP();
	}
}

void recover_from_task_fault() {
	while (true) {
		__NOP();
	}
}
