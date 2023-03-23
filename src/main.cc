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

	UserSettingsStorage settings_storage{sd};

	Flags flags;
	SampleList samples;
	BankManager banks{samples};
	Params params{controls, flags, system_calibrations, settings_storage.settings, banks};

	// Load sample index file (map files to sample slots and banks)
	SampleIndexLoader index_loader{sd, samples, banks, flags};
	{
		Timekeeper startup_animation{Brain::param_update_task_conf, [&params] { params.startup_animation(); }};
		startup_animation.start();
		index_loader.load_all_banks();
		startup_animation.stop();
	}

	Sampler sampler{params, flags, sd, banks};
	AudioStream audio_stream([&sampler, &params](const AudioInBlock &in, AudioOutBlock &out) {
		Debug::Pin0::high();
		params.update();
		sampler.audio.update(in, out);
		Debug::Pin0::low();
	});

	sampler.start();
	audio_stream.start();

	while (true) {
		sampler.update();
		// settings_storage.handle_events();

		__NOP();
	}
}

void recover_from_task_fault(void) { __BKPT(); }
