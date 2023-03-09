#include "audio_stream.hh"
#include "conf/board_conf.hh"
#include "hardware_tests/adc.hh"
#include "hardware_tests/buttons.hh"
#include "hardware_tests/gate_ins.hh"
#include "hardware_tests/leds.hh"
#include "hardware_tests/util.hh"
#include "libhwtests/CodecCallbacks.hh"
#include "stm32xx.h"
#include "util/zip.hh"

namespace SamplerKit::HWTests
{

void all_lights_off() {
	Board::PlayLED{}.set_color(Colors::off);
	Board::RevLED{}.set_color(Colors::off);
	Board::BankLED{}.set_color(Colors::off);
}

void run(Controls &controls) {
	all_lights_off();
	Util::pause_until_button_released();
	Util::flash_mainbut_until_pressed();

	// Press Play to cycle through LEDs one at a time
	// then each button white
	TestLEDs ledtester;
	ledtester.run_test();

	// Press each button once
	TestButtons buttontester;
	buttontester.run_test();

	// Audio out and End Out test
	// Endout shoudl see 8V square wave 750HZ
	// OutLeft should see -10V to +10V right-leaning tri 500Hz
	// OutRight should see -10V to +10V left-leaning tri 3.7kHz (3.5kHz?)
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

	Util::flash_mainbut_until_pressed();

	// Audio input test:
	// Should see nothing on Out L
	// Then patch Out R to In R, should see the 3.4kHz 20Vpp wave
	// Then patch Out R to In L, should see the same wave but inverted (right-leaning)
	audio.set_callback([&oscR](const AudioStreamConf::AudioInBlock &in, AudioStreamConf::AudioOutBlock &out) {
		for (auto [i, o] : zip(in, out)) {
			o.chan[0] = oscR.update() * 0x7FFFFF;
			o.chan[1] = i.chan[0] - i.chan[1];
		}
	});

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
	TestADCs adctester{controls};
	adctester.run_test();

	all_lights_off();

	// Patch End Out into Play Trig, then Rev Trig
	TestGateIns gateintester;
	gateintester.run_test();

	Util::flash_mainbut_until_pressed();

	// SD Card
}
} // namespace SamplerKit::HWTests
