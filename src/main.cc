#include "audio_stream.hh"
#include "conf/board_conf.hh"
#include "controls.hh"
#include "debug.hh"
// #include "delay_buffer.hh"
#include "drivers/timekeeper.hh"
// #include "looping_delay.hh"
#include "sdcard.hh"
#include "system.hh"
// #include "timer.hh"
#include "hardware_tests/hardware_tests.hh"
#include "test_audio.hh"

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

	Sdcard sd;
	sd.reload_disk();

	// Flags flags;
	// Params params{controls, flags, timer};
	// DelayBuffer &audio_buffer = get_delay_buffer();
	// LoopingDelay looping_delay{params, flags, audio_buffer};
	// TestAudio sampler; //{params};
	// AudioStream audio([&sampler](const AudioInBlock &in, AudioOutBlock &out) { sampler.update(in, out); });

	mdrivlib::Timekeeper params_update_task(Board::param_update_task_conf, [&controls]() { controls.update(); });

	// TODO: Make Params thread-safe:
	// Use double-buffering (two Params structs), and LoopingDelay is constructed with a Params*
	// And right before looping_delay.update(), call params.load_updated_values()

	__HAL_DBGMCU_FREEZE_TIM6();
	// Tasks:
	// LED PWM update(TIM2) + Button LED(TIM10) 4.8kHz + 200Hz
	// SD Card read: 1.4kHz (TIM7)
	// Trig Jack(TIM5)? 12kHz

	// timer.start();
	controls.play_led.breathe(Colors::white, 0.1);
	controls.rev_led.breathe(Colors::white, 0.1);
	controls.bank_led.breathe(Colors::white, 0.1);
	params_update_task.start();

	// audio.start();

	while (true) {
		__NOP();
	}
}

void recover_from_task_fault(void) { __BKPT(); }
