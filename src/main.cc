#include "audio_stream.hh"
#include "bank.hh"
#include "calibration_storage.hh"
#include "conf/board_conf.hh"
#include "controls.hh"
#include "debug.hh"
#include "drivers/timekeeper.hh"
#include "flags.hh"
#include "hardware_tests/hardware_tests.hh"
#include "params.hh"
#include "sampler.hh"
#include "sampler_audio.hh"
#include "sampler_loader.hh"
#include "sdcard.hh"
#include "system.hh"
#include "test_audio.hh"
#include "user_settings_storage.hh"

namespace
{
// Initialize the system before main()
SamplerKit::System _init;
} // namespace

void main() {
	using namespace SamplerKit;
	using namespace mdrivlib;
	using AudioInBlock = AudioStreamConf::AudioInBlock;
	using AudioOutBlock = AudioStreamConf::AudioOutBlock;

	Controls controls;

	if (Board::PlayButton::PinT::read() && Board::RevButton::PinT::read()) {
		HWTests::run(controls);
	}

	CalibrationStorage system_calibrations;
	auto fw_version = system_calibrations.load_flash_params();
	// check_calibration_mode();

	Sdcard sd;
	sd.reload_disk();

	Flags flags;
	Params params{controls, flags, system_calibrations};

	UserSettingsStorage settings_storage{params.settings, sd};

	SampleList samples;
	BankManager banks{samples};

	// TODO: show progress of loading sample index
	SampleBankFiles sample_bank_files{sd, samples, banks};
	sample_bank_files.load_all_banks();

	///////////Sampler class:
	std::array<CircularBuffer, NumSamplesPerBank> play_buff;
	uint32_t g_error = 0;
	SamplerModes sampler_modes{params, flags, sd, samples, banks, play_buff, g_error};

	SamplerAudio sampler{params, samples, flags, play_buff, sampler_modes};
	AudioStream audio([&sampler](const AudioInBlock &in, AudioOutBlock &out) { sampler.update(in, out); });

	SampleLoader loader{sampler_modes, params, flags, sd, samples, banks, play_buff, g_error};
	//////////

	// TODO Tasks:
	// Trig Jack(TIM5)? 12kHz

	Timekeeper params_update_task(Board::param_update_task_conf, [&params]() { params.update(); });
	params_update_task.start();
	loader.start();
	audio.start();

	while (true) {
		sampler_modes.process_mode_flags();
		loader.update();

		__NOP();
	}
}

void recover_from_task_fault(void) { __BKPT(); }
