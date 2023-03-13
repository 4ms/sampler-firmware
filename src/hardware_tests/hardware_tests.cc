#include "audio_stream.hh"
#include "conf/board_conf.hh"
#include "conf/brain_conf.hh"
#include "drivers/ram_test.hh"
#include "hardware_tests/adc.hh"
#include "hardware_tests/buttons.hh"
#include "hardware_tests/gate_ins.hh"
#include "hardware_tests/leds.hh"
#include "hardware_tests/sd.hh"
#include "hardware_tests/util.hh"
#include "libhwtests/CodecCallbacks.hh"
#include "stm32xx.h"
#include "util/zip.hh"

extern "C" int printf_(const char *format, ...);

namespace SamplerKit::HWTests
{

void all_lights_off() {
	Board::PlayLED{}.set_color(Colors::off);
	Board::RevLED{}.set_color(Colors::off);
	Board::BankLED{}.set_color(Colors::off);
}

void run(Controls &controls) {
	printf_("\n%sSampler Kit Hardware Test%s\n", "\x1b[31m", "\033[0m");

	// SD Card
	printf_("Initializing SD Card periph\n");
	TestSDCard sdtest;
	// sdtest.run_test();
	sdtest.run_fatfs_test();

	printf_("Press button to continue\n");
	all_lights_off();
	Util::pause_until_button_released();
	Util::flash_mainbut_until_pressed();

	// Press Play to cycle through LEDs one at a time
	// then each button white
	printf_("LED Test\n");
	TestLEDs ledtester;
	ledtester.run_test();

	// Press each button once
	printf_("Skipping Button Test\n");
	TestButtons buttontester;
	buttontester.run_test();

	// Audio out and End Out test
	// Endout shoudl see 8V square wave 750HZ
	// OutLeft should see -10V to +10V right-leaning tri 500Hz
	// OutRight should see -10V to +10V left-leaning tri 3.7kHz (3.5kHz?)
	printf_("\n%sAudio Output Test%s\n", "\x1b[31m", "\033[0m");
	SkewedTriOsc oscL{500, 0.3, 1, -1, 0, 48000};
	SkewedTriOsc oscR{3700, 0.85, 1, -1, 0, 48000};
	AudioStream audio([&oscL, &oscR](const AudioStreamConf::AudioInBlock &in, AudioStreamConf::AudioOutBlock &out) {
		static bool endout;
		for (auto &o : out) {
			o.chan[0] = oscR.update() * 0x7FFFFF;
			o.chan[1] = oscL.update() * 0x7FFFFF;
		}
		Board::EndOut::set(endout);
		endout = !endout;
	});
	audio.start();
	printf_("Verify:\n");
	printf_("  1) Out Left: 500Hz right-leaning triangle, -10V to +10V [+/- 0.3V]\n");
	printf_("  2) Out Right: 3700Hz left-leaning triangle, -10V to +10V [+/- 0.3V]\n");
	printf_("  3) End Out: 750Hz square wave, 0V to +8V [+/- 0.5V]\n");
	printf_("Press button when verified\n");

	Util::flash_mainbut_until_pressed();

	// Audio input test:
	// Should see nothing on Out L
	// Then patch Out R to In R, should see the 3.4kHz 20Vpp wave
	// Then patch Out R to In L, should see the same wave but inverted (right-leaning)
	printf_("Audio Input Test\n");
	audio.set_callback([&oscR](const AudioStreamConf::AudioInBlock &in, AudioStreamConf::AudioOutBlock &out) {
		for (auto [i, o] : zip(in, out)) {
			o.chan[0] = oscR.update() * 0x7FFFFF;
			o.chan[1] = i.chan[0] - i.chan[1];
		}
	});
	printf_("  1) Patch Out L to scope, verify no signal\n");
	printf_("  2) Patch Out R to In R, verify left-leaning 3.4kHz 20Vpp wave [+/- 0.3V] on Out L\n");
	printf_("  3) Unpatch In R. Patch Out R to In L, verify right-leaning 3.4kHz 20Vpp wave [+/- 0.3V] on Out L\n");
	printf_("Press button when verified\n");

	Util::flash_mainbut_until_pressed();

	// Test waveforms to test CV jacks
	CenterFlatRamp test_waveform_0_5{2., 0.2, -4000000, 0, 0, 48000};
	CenterFlatRamp test_waveform_n5_5{2., 0.2, 4000000, -4000000, 0, 48000};
	audio.set_callback([&](const AudioStreamConf::AudioInBlock &in, AudioStreamConf::AudioOutBlock &out) {
		for (auto &o : out) {
			o.chan[1] = test_waveform_0_5.update();
			o.chan[0] = test_waveform_n5_5.update();
		}
	});

	// Turn each pot
	// Patch Out L into Pitch CV, then Out R into each of the other 4 CVs
	printf_("Pot and CV jack test\n");
	TestADCs adctester{controls};
	adctester.run_test();

	all_lights_off();

	// Patch End Out into Play Trig, then Rev Trig
	TestGateIns gateintester;
	gateintester.run_test();

	all_lights_off();
	Util::flash_mainbut_until_pressed();

	// RAM Test
	controls.bank_led.set_color(Colors::white);
	auto err = mdrivlib::RamTest::test(Brain::MemoryStartAddr, Brain::MemorySizeBytes);
	if (err) {
		while (1) {
			controls.rev_led.set_color(Colors::red);
			controls.bank_led.set_color(Colors::red);
			controls.play_led.set_color(Colors::red);
			HAL_Delay(200);
			all_lights_off();
			HAL_Delay(200);
		}
	}
	controls.rev_led.set_color(Colors::white);
	controls.bank_led.set_color(Colors::off);

	Util::flash_mainbut_until_pressed();

	all_lights_off();
}
} // namespace SamplerKit::HWTests
