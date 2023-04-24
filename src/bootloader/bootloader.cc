#include "drivers/stm32xx.h"

#define USING_FSK
// #define USING_QPSK

#include "bl_utils.hh"
#include "bootloader/animation.hh"
#include "bootloader/buttons.hh"
#include "bootloader/gate_input.hh"
#include "bootloader/leds.hh"
#include "conf/bootloader_settings.hh"
#include "conf/flash_layout.hh"
#include "system.hh"
// #include "dig_inouts.hh"
// #include "flash.hh"

#ifdef USING_QPSK
#include "stm_audio_bootloader/qpsk/demodulator.h"
#include "stm_audio_bootloader/qpsk/packet_decoder.h"
#else
#include "stm_audio_bootloader/fsk/demodulator.h"
#include "stm_audio_bootloader/fsk/packet_decoder.h"
#endif

namespace SamplerKit::Bootloader
{

using namespace stmlib;
using namespace stm_audio_bootloader;

#ifdef USING_QPSK
constexpr float kModulationRate = 6000.0;
constexpr float kBitRate = 12000.0;
constexpr float kSampleRate = 48000.0;
#else
constexpr uint32_t kSampleRate = BootloaderConf::SampleRate;	  //-s
constexpr uint32_t kPausePeriod = BootloaderConf::Encoding.blank; //-b
constexpr uint32_t kOnePeriod = BootloaderConf::Encoding.one;	  //-n
constexpr uint32_t kZeroPeriod = BootloaderConf::Encoding.zero;	  //-z
																  //-p must be 256 (set in fsk/packet_decoder.h)
#endif

constexpr uint32_t kStartExecutionAddress = AppFlashAddr;
constexpr uint32_t kStartReceiveAddress = BootloaderReceiveAddr;
constexpr uint32_t kBlkSize = BootloaderConf::ReceiveSectorSize; // Flash page size, -g

constexpr uint16_t kPacketsPerBlock = kBlkSize / kPacketSize; // kPacketSize=256
uint8_t recv_buffer[kBlkSize];

volatile uint32_t systmr = 0;
PacketDecoder decoder;
Demodulator demodulator;

uint16_t packet_index;
uint16_t discard_samples = 8000;
uint32_t current_flash_address;

constexpr uint32_t thresh_stepsize = 10;
constexpr uint32_t thresh_min = 2080;
constexpr uint32_t thresh_steps = 25;
uint32_t gatein_threshold = 2200;

enum UiState { UI_STATE_WAITING, UI_STATE_RECEIVING, UI_STATE_ERROR, UI_STATE_WRITING, UI_STATE_DONE };
UiState ui_state;

static void animate_until_button_pushed(Animations animation_type, Button button);
static void update_LEDs();
static void init_reception();
static void delay(uint32_t tm);
static bool write_buffer();
static void new_block();
static void new_packet();
static void set_threshold_led();

void main() {
	uint32_t symbols_processed = 0;
	uint32_t dly = 0, button_debounce = 0;
	uint8_t do_bootloader;
	uint8_t symbol;
	PacketDecoderState state;
	bool rcv_err;
	uint8_t exit_updater = false;

	mdrivlib::System::SetVectorTable(BootloaderFlashAddr);
	mdrivlib::SystemClocks::init_clocks(osc_conf, clk_conf, rcc_periph_conf);

	delay(300);

	init_leds();
	init_buttons();
	init_gate_in();

	animate(ANI_RESET);

	dly = 32000;
	while (dly--) {
		if (button_pushed(Button::Rev) && button_pushed(Button::Bank))
			button_debounce++;
		else
			button_debounce = 0;
	}
	do_bootloader = (button_debounce > 15000) ? 1 : 0;

	delay(100);

	// Debug:
	//  DigIO::ClockBusOut init;
	//  mdrivlib::FPin<mdrivlib::GPIO::E, mdrivlib::PinNum::_3, mdrivlib::PinMode::Output> debug2;
	//  debug2.low();
	//  DigIO::ClockBusOut::low();

	if (do_bootloader) {
		init_reception();

		start_reception(kSampleRate, []() {
			bool sample = gate_in_read(gatein_threshold);
			if (!discard_samples) {
				demodulator.PushSample(sample ? 1 : 0);
			} else {
				--discard_samples;
			}
		});

		uint32_t button1_exit_armed = 0;
		uint32_t cycle_but_armed = 0;

		while (button_pushed(Button::Rev) || button_pushed(Button::Bank))
			;

		delay(300);
		set_threshold_led();

		while (!exit_updater) {
			rcv_err = false;

			while (demodulator.available() && !rcv_err && !exit_updater) {
				symbol = demodulator.NextSymbol();
				state = decoder.ProcessSymbol(symbol);
				symbols_processed++;

				switch (state) {
					case PACKET_DECODER_STATE_SYNCING:
						animate(ANI_SYNC);
						break;

					case PACKET_DECODER_STATE_OK:
						ui_state = UI_STATE_RECEIVING;
						memcpy(recv_buffer + (packet_index % kPacketsPerBlock) * kPacketSize,
							   decoder.packet_data(),
							   kPacketSize);
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

					case PACKET_DECODER_STATE_ERROR_SYNC:
						rcv_err = true;
						break;

					case PACKET_DECODER_STATE_ERROR_CRC:
						rcv_err = true;
						break;

					case PACKET_DECODER_STATE_END_OF_TRANSMISSION:
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
						delay(100);
						break;

					default:
						break;
				}
			}
			if (rcv_err) {
				ui_state = UI_STATE_ERROR;
				animate_until_button_pushed(ANI_FAIL_ERR, Button::Play);
				animate(ANI_RESET);
				delay(100);
				init_reception();
				exit_updater = false;
			}

			if (button_pushed(Button::Rev)) {
				if (cycle_but_armed) {
					if (packet_index == 0) {
						gatein_threshold += thresh_stepsize;
						if (gatein_threshold >= (thresh_min + thresh_steps * thresh_stepsize))
							gatein_threshold = thresh_min;
						set_threshold_led();
					} else {
						delay(100);
						init_reception();
					}
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

	mdrivlib::System::reset_buses();
	mdrivlib::System::reset_RCC();
	mdrivlib::System::jump_to(kStartExecutionAddress);
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
		delay(1);
		animate(animation_type);
	}
	while (button_pushed(button)) {
		delay(1);
	}
}

void delay(uint32_t ticks) {
	uint32_t i = systmr;
	while ((systmr - i) < ticks) {
		;
	}
}

} // namespace SamplerKit::Bootloader

extern "C" void SysTick_Handler(void) {
	systmr = systmr + 1;
	update_LEDs();
}

extern "C" void NMI_Handler() { __BKPT(); }
extern "C" void HardFault_Handler() { __BKPT(); }
extern "C" void MemManage_Handler() { __BKPT(); }
extern "C" void BusFault_Handler() { __BKPT(); }
extern "C" void UsageFault_Handler() { __BKPT(); }
extern "C" void SVC_Handler() { __BKPT(); }
extern "C" void DebugMon_Handler() { __BKPT(); }
extern "C" void PendSV_Handler() { __BKPT(); }
