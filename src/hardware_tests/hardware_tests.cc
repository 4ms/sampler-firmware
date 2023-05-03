#include "audio_stream.hh"
#include "brain_conf.hh"
#include "calibration_storage.hh"
#include "conf/board_conf.hh"
#include "console.hh"
#include "drivers/ram_test.hh"
#include "drivers/stm32xx.h"
#include "hardware_tests/adc.hh"
#include "hardware_tests/buttons.hh"
#include "hardware_tests/gate_ins.hh"
#include "hardware_tests/leds.hh"
#include "hardware_tests/sd.hh"
#include "hardware_tests/util.hh"
#include "libhwtests/CodecCallbacks.hh"
#include "printf.h"
#include "util/term_codes.hh"
#include "util/zip.hh"

namespace SamplerKit::HWTests
{

void all_lights_off() {
	Board::PlayLED{}.set_color(Colors::off);
	Board::RevLED{}.set_color(Colors::off);
	Board::BankLED{}.set_color(Colors::off);
}

void print_test_name(std::string_view nm) {
	printf_("\n-------------------------------------\n");
	printf_("%s%.64s%s\n", Term::BoldYellow, nm.data(), Term::Normal);
}
void print_press_button_msg() { printf_("%sPress button to continue%s\n", Term::BlinkGreen, Term::Normal); }
void print_error(std::string_view err) { printf_("%s%.255s%s\n", Term::BoldRed, err.data(), Term::Normal); }

void run(Controls &controls, CalibrationStorage &cal_storage) {
	printf_("\n\n%sSampler Kit Hardware Test%s\n", Term::BoldGreen, Term::Normal);

	//////////////////////////////
	print_test_name("SD Card Test");
	TestSDCard sdtest;
	// sdtest.run_test();
	sdtest.run_fatfs_test();

	all_lights_off();
	Util::pause_until_button_released();

	print_press_button_msg();
	Util::flash_mainbut_until_pressed();

	//////////////////////////////
	print_test_name("LED Test");
	printf_("Press the Play button to verify each LED. You'll see red=>green=>blue\n");
	printf_("The LEDs will each turn white for you to verify color balance\n");
	TestLEDs ledtester;
	ledtester.run_test();

	printf_("If the LEDs need calibration, press and hold Play for 3 seconds Reverse. Otherwise press Play briefly to "
			"continue\n");
	Util::pause_until_button_released();
	if (Util::check_for_longhold_button()) {
		printf_("Calibrating LEDs...\n");
		// TODO:
		// led_cal{controls.
		LEDCalibrator led_cal;
		while (true) {
			if (controls.bank_button.is_just_pressed())
				led_cal.select_led(0);
			if (controls.play_button.is_just_pressed())
				led_cal.select_led(1);
			if (controls.rev_button.is_just_pressed())
				led_cal.select_led(2);

			auto red = controls.read_pot(PitchPot);
			auto green = controls.read_pot(StartPot);
			auto blue = controls.read_pot(LengthPot);
			led_cal.update(red, green, blue);

			if (led_cal.result() == LedCalibration::Save) {
				cal_storage.save_flash_params();
				break;
			}
			if (led_cal.result() == LedCalibration::Cancel) {
				break;
			}
		}
		cal_storage.save_flash_params();
	}

	//////////////////////////////
	print_test_name("Button Test");
	printf_("Press each button once when it lights up\n");
	all_lights_off();
	TestButtons buttontester;
	buttontester.run_test();

	//////////////////////////////
	print_test_name("Audio Output Test");
	SinOsc oscL{440.f, 2, 0, 48000};
	SinOsc oscR{2637.02f, 2, 0, 48000};
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
	printf_("  1) Out Left: 440Hz sine (A4), -10V to +10V [+/- 0.3V]\n");
	printf_("  2) Out Right: 2.637kHz sine (E7), -10V to +10V [+/- 0.3V]\n");
	printf_("  3) End Out: 1.5kHz square wave, 0V to +8V [+/- 0.5V]\n");

	print_press_button_msg();
	Board::BankLED{}.set_color(Colors::orange);
	Util::flash_mainbut_until_pressed();
	Board::BankLED{}.set_color(Colors::off);

	//////////////////////////////
	print_test_name("Audio Input Test");
	SinOsc osc3{377, 1.00f, 0.f, 48000};
	audio.set_callback([&osc3](const AudioStreamConf::AudioInBlock &in, AudioStreamConf::AudioOutBlock &out) {
		static uint8_t endout_ctr = 0;
		static bool endout = false;
		for (auto [i, o] : zip(in, out)) {
			o.chan[0] = osc3.update() * 0x7FFFFF;
			o.chan[1] = i.chan[0] - i.chan[1];
		}
		if (((endout_ctr++) & 0b11) == 0b00)
			endout = !endout;
		Board::EndOut::set(endout);
	});
	printf_("  1) Patch Out L to scope, verify no signal\n");
	printf_("  2) Patch Out R to In R, verify 377Hz sine 8Vpp wave [+/- 0.4V] on Out L\n");
	printf_("  3) Patch EndOut to In L (keep other cable patched), verify beat freq 2Hz on Out L\n");

	print_press_button_msg();
	Board::RevLED{}.set_color(Colors::orange);
	Util::flash_mainbut_until_pressed();
	Board::RevLED{}.set_color(Colors::off);
	controls.play_button.clear_events();

	//////////////////////////////
	print_test_name("Pot and CV jack Test");
	controls.start();

	printf_("Turn each pot from low to high to center\n");
	printf_("After the pots, patch Out L into Pitch CV (bi-polar CV)\n");
	printf_("Then patch Out R into the other CV jacks (uni-polar CV)\n");

	// center -2'200'000 = 2.35V;
	// center -2'250'000 = 2.41V
	// est center -2'350'000 = 2.5V
	// 100'000 = ADC 10, ~190mV
	// -4'600'000 > ADC 4095, ~4.97V
	CenterFlatRamp test_waveform_0_5{1., 0.3, -4'600'000, 100'000, 0, 48000};
	CenterFlatRamp test_waveform_n5_5{1., 0.3, 4'600'000, -4'600'000, 0, 48000};
	audio.set_callback([&](const AudioStreamConf::AudioInBlock &in, AudioStreamConf::AudioOutBlock &out) {
		for (auto &o : out) {
			o.chan[0] = test_waveform_0_5.update();	 // R
			o.chan[1] = test_waveform_n5_5.update(); // L
		}
	});

	TestADCs adctester{controls};
	adctester.run_test();

	all_lights_off();
	controls.play_button.clear_events();

	//////////////////////////////
	print_test_name("Gate Input Test");
	printf_("Patch End Out into Play Trig, then Rev Trig\n");
	TestGateIns gateintester{controls};
	gateintester.run_test();

	all_lights_off();

	//////////////////////////////
	print_test_name("RAM Test (automatic)");
	printf_("If this takes longer than 20 seconds then RAM Test fails.\n");
	Board::BankLED{}.set_color(Colors::white);
	auto err = mdrivlib::RamTest::test(Brain::MemoryStartAddr, Brain::MemorySizeBytes);
	if (err) {
		print_error("RAM Test Failed: readback did not match\n");
		while (1) {
			Board::PlayLED{}.set_color(Colors::red);
			Board::BankLED{}.set_color(Colors::red);
			Board::RevLED{}.set_color(Colors::red);
			HAL_Delay(200);
			all_lights_off();
			HAL_Delay(200);
		}
	}
	Board::RevLED{}.set_color(Colors::white);
	Board::BankLED{}.set_color(Colors::off);

	//////////////////////////////
	printf_("Hardware Test Complete.\n");

	printf_("Please reboot\n"); //, or system will automatically reboot in 5 seconds\n.");
	all_lights_off();

	while (true)
		;
}
} // namespace SamplerKit::HWTests
