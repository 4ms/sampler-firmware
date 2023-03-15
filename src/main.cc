#include "audio_stream.hh"
#include "conf/board_conf.hh"
#include "controls.hh"
#include "debug.hh"
#include "flags.hh"
// #include "delay_buffer.hh"
#include "drivers/timekeeper.hh"
// #include "looping_delay.hh"
#include "params.hh"
#include "sdcard.hh"
#include "system.hh"
// #include "timer.hh"
#include "hardware_tests/hardware_tests.hh"
#include "test_audio.hh"
#include "user_settings_storage.hh"
#include "user_system_settings.hh"

namespace
{
// Initialize the system before main()
SamplerKit::System _init;
} // namespace

void main() {
	using namespace SamplerKit;
	using AudioInBlock = AudioStreamConf::AudioInBlock;
	using AudioOutBlock = AudioStreamConf::AudioOutBlock;

	Controls controls;

	controls.start();
	controls.update();
	if (controls.play_button.is_pressed() && controls.rev_button.is_pressed()) {
		HWTests::run(controls);
	}

	UserSystemSettings sys_settings;
	auto fw_version = sys_settings.load_flash_params();

	// check_calibration_mode();

	Sdcard sd;
	sd.reload_disk();

	Flags flags;
	Params params{controls, flags};

	UserSettingsStorage settings_storage{sd, params.settings};

	TestAudio sampler; //{params, flags};
	AudioStream audio([&sampler](const AudioInBlock &in, AudioOutBlock &out) { sampler.update(in, out); });

	mdrivlib::Timekeeper params_update_task(Board::param_update_task_conf, [&controls]() { controls.update(); });

	// TODO: Make Params thread-safe:
	// Use double-buffering (two Params structs), and LoopingDelay is constructed with a Params*
	// And right before looping_delay.update(), call params.load_updated_values()

	__HAL_DBGMCU_FREEZE_TIM6();

	// Tasks:
	// SD Card read: 1.4kHz (TIM7)
	// Trig Jack(TIM5)? 12kHz

	// timer.start();
	params_update_task.start();

	// audio.start();

	while (true) {
		__NOP();
	}
}

void recover_from_task_fault(void) { __BKPT(); }
