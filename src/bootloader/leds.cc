#include "bootloader/leds.hh"
#include "pwm_pins.hh"

template<mdrivlib::PinNoInit LedPin>
using LEDPin = mdrivlib::FPin<LedPin.gpio, LedPin.pin, mdrivlib::PinMode::Output, mdrivlib::PinPolarity::Inverted>;

using PingR = LEDPin<LEDPins::Pingbut_r>;
using PingG = LEDPin<LEDPins::Pingbut_g>;
using PingB = LEDPin<LEDPins::Pingbut_b>;

using CycleR = LEDPin<LEDPins::Cyclebut_r>;
using CycleG = LEDPin<LEDPins::Cyclebut_g>;
using CycleB = LEDPin<LEDPins::Cyclebut_b>;

using EnvAR = LEDPin<LEDPins::EnvA_r>;
using EnvAG = LEDPin<LEDPins::EnvA_g>;
using EnvAB = LEDPin<LEDPins::EnvA_b>;

using EnvBR = LEDPin<LEDPins::EnvB_r>;
using EnvBG = LEDPin<LEDPins::EnvB_g>;
using EnvBB = LEDPin<LEDPins::EnvB_b>;

void init_leds() {
	PingR pingr;
	PingG pingg;
	PingB pingb;

	CycleR cycler;
	CycleG cycleg;
	CycleB cycleb;

	EnvAR envar;
	EnvAG envag;
	EnvAB envab;

	EnvBR envbr;
	EnvBG envbg;
	EnvBB envbb;

	set_rgb_led(RgbLeds::Ping, Palette::Black);
	set_rgb_led(RgbLeds::Cycle, Palette::Black);
	set_rgb_led(RgbLeds::EnvA, Palette::Black);
	set_rgb_led(RgbLeds::EnvB, Palette::Black);
}

void set_rgb_led(RgbLeds rgb_led_id, Palette color_id) {

	uint32_t color = static_cast<uint32_t>(color_id);

	if (rgb_led_id == RgbLeds::Ping) {
		PingR::set(color & 0b100);
		PingG::set(color & 0b010);
		PingB::set(color & 0b001);
	} else if (rgb_led_id == RgbLeds::Cycle) {
		CycleR::set(color & 0b100);
		CycleG::set(color & 0b010);
		CycleB::set(color & 0b001);
	} else if (rgb_led_id == RgbLeds::EnvA) {
		EnvAR::set(color & 0b100);
		EnvAG::set(color & 0b010);
		EnvAB::set(color & 0b001);
	} else if (rgb_led_id == RgbLeds::EnvB) {
		EnvBR::set(color & 0b100);
		EnvBG::set(color & 0b010);
		EnvBB::set(color & 0b001);
	}
}
