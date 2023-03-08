#include "audio_stream.hh"
#include "conf/board_conf.hh"
#include "controls.hh"
#include "debug.hh"
// #include "delay_buffer.hh"
#include "drivers/timekeeper.hh"
// #include "looping_delay.hh"
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

	Debug::Pin0{};
	Debug::Pin1{};
	Debug::Pin2{};
	Debug::Pin3{};

	Controls controls;
	// Flags flags;
	// Timer timer;
	// Params params{controls, flags, timer};
	// DelayBuffer &audio_buffer = get_delay_buffer();
	// LoopingDelay looping_delay{params, flags, audio_buffer};
	TestAudio sampler; //{params};
	AudioStream audio([&sampler](const AudioInBlock &in, AudioOutBlock &out) { sampler.update(in, out); });

	// mdrivlib::Timekeeper params_update_task(Board::param_update_task_conf, [&params]() { params.update(); });

	// TODO: Make Params thread-safe:
	// Use double-buffering (two Params structs), and LoopingDelay is constructed with a Params*
	// And right before looping_delay.update(), call params.load_updated_values()

	__HAL_DBGMCU_FREEZE_TIM6();

	// timer.start();
	controls.start();
	// params_update_task.start();
	audio.start();

	HWTests::run(controls);

	while (true) {
		__NOP();
	}
}
void recover_from_task_fault(void) { __BKPT(); }
