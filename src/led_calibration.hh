#pragma once
#include "controls.hh"
#include "flags.hh"
#include "log.hh"
#include "util/colors.hh"
#include <cmath>

namespace SamplerKit
{

struct LEDCalibrator {
	LEDCalibrator(Controls &controls)
		: controls{controls} {}

	void reset() {}

	bool run() {
		// Start with bank LED
		cur_led_adj = &adjustments.bank;

		while (true) {
			controls.bank_button.update();
			controls.play_button.update();
			controls.rev_button.update();

			// Select LED by pressing a button
			if (controls.bank_button.is_just_pressed())
				cur_led_adj = &adjustments.bank;

			if (controls.play_button.is_just_pressed())
				cur_led_adj = &adjustments.play;

			if (controls.rev_button.is_just_pressed())
				cur_led_adj = &adjustments.rev;

			// Read R,G,B adjustments from pots
			cur_led_adj->r = controls.read_pot(PitchPot) >> 4;
			cur_led_adj->g = controls.read_pot(StartPot) >> 4;
			cur_led_adj->b = controls.read_pot(LengthPot) >> 4;

			// Shine each LED with adjusted white color
			// TODO: flash current led?
			controls.bank_led.set_color(Colors::white.adjust(adjustments.bank));
			controls.play_led.set_color(Colors::white.adjust(adjustments.play));
			controls.rev_led.set_color(Colors::white.adjust(adjustments.rev));

			// Long hold Play to exit and save
			if (controls.play_button.how_long_held_pressed() > 1000)
				return true;

			// Long hold Bank or Rev to exit/cancel
			if (controls.rev_button.how_long_held_pressed() > 1000)
				return false;

			if (controls.bank_button.how_long_held_pressed() > 1000)
				return false;
		}
		return false;
	}

	struct Adjustments {
		Color::Adjustment bank;
		Color::Adjustment play;
		Color::Adjustment rev;
	};

	Adjustments get_adjustments() { return adjustments; }

private:
	Controls &controls;
	Adjustments adjustments{{128, 128, 128}, {128, 128, 128}, {128, 128, 128}};
	Color::Adjustment *cur_led_adj = &adjustments.bank;
};

} // namespace SamplerKit
