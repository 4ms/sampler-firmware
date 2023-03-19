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

	controls.start();
	controls.update();
	if (controls.play_button.is_pressed() && controls.rev_button.is_pressed()) {
		HWTests::run(controls);
	}

	CalibrationStorage system_calibrations;
	auto fw_version = system_calibrations.load_flash_params();
	// check_calibration_mode();

	Sdcard sd;
	sd.reload_disk();

	Flags flags;
	Params params{controls, flags, system_calibrations};

	UserSettingsStorage settings_storage{params.settings};

	SamplerAudio sampler{params};
	AudioStream audio([&sampler](const AudioInBlock &in, AudioOutBlock &out) { sampler.update(in, out); });

	Timekeeper params_update_task(Board::param_update_task_conf, [&controls]() { controls.update(); });

	SampleList samples;
	BankManager{samples};

	// load_all_banks();
	// set startupbank

	Sampler sampler_bg{params, flags, sd, samples};

	// TODO Tasks:
	// SD Card read: 1.4kHz (TIM7)
	// Trig Jack(TIM5)? 12kHz

	params_update_task.start();
	audio.start();

	while (true) {
		sampler_bg.process_mode_flags();
		__NOP();
	}
}

void recover_from_task_fault(void) { __BKPT(); }
