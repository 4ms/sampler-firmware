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
	Timekeeper params_update_task(Board::param_update_task_conf, [&params]() { params.update(); });
	params_update_task.start();

	UserSettingsStorage settings_storage{params.settings, sd};

	// Load sample index file (map files to sample slots and banks)
	SampleList samples;
	BankManager banks{samples};
	SampleIndexLoader index_loader{sd, samples, banks, flags};
	index_loader.load_all_banks();

	Sampler sampler{params, flags, sd, banks};
	AudioStream audio_stream([&sampler](const AudioInBlock &in, AudioOutBlock &out) { sampler.audio.update(in, out); });

	sampler.start();
	audio_stream.start();

	// TODO Tasks:
	// Trig Jack(TIM5)? 12kHz
	while (true) {
		sampler.update();

		__NOP();
	}
}

void recover_from_task_fault(void) { __BKPT(); }
