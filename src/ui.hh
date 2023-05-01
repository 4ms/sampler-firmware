#pragma once

#include "bank_blink.hh"
#include "controls.hh"
#include "flags.hh"
#include "palette.hh"
#include "settings.hh"

namespace SamplerKit
{
struct Ui {
	Flags &flags;
	Controls &controls;

	Ui(Flags &flags, Controls &controls)
		: flags{flags}
		, controls{controls} {}

	struct LedCriteria {
		OperationMode op_mode;
		PlayStates play_state;
		RecStates rec_state;
		bool reverse;
		bool looping;
		uint32_t bank;
	};

	Color last_play_color = Colors::off;
	Color last_rev_color = Colors::off;
	Color last_bank_color = Colors::off;

	void update_leds(LedCriteria state) {
		const OperationMode &op_mode = state.op_mode;
		const PlayStates &play_state = state.play_state;
		const RecStates &rec_state = state.rec_state;
		const bool &reverse = state.reverse;
		const bool &looping = state.looping;
		const uint32_t &bank = state.bank;

		Color rev_color;
		Color play_color;
		Color bank_color;

		if (op_mode == OperationMode::Playback) {
			rev_color = reverse ? Colors::blue : Colors::off;

			if (play_state != PlayStates::SILENT && play_state != PlayStates::PREBUFFERING) {
				play_color = looping ? SamplerColors::cyan : SamplerColors::green;
			} else
				play_color = Colors::off;
		}
		if (op_mode == OperationMode::Record) {
			rev_color = Colors::off;
			if (rec_state == RecStates::REC_OFF) {
				play_color = Colors::off;
				controls.play_led.breathe(Colors::red, 4.f);
			} else {
				play_color = Colors::red;
				controls.play_led.reset_breathe();
			}
		}

		bank_color = blink_bank(bank, HAL_GetTick()) ? BankColors[bank % 10] : Colors::off;

		if (op_mode == OperationMode::Calibrate) {
			rev_color = Colors::purple;
			play_color = Colors::purple;
			bank_color = Colors::purple;
		}
		if (op_mode == OperationMode::SystemMode) {
			// TODO: calibration and system modes
		}

		// Output to the LEDs
		if (last_play_color != play_color) {
			last_play_color = play_color;
			controls.play_led.set_base_color(play_color);
		}
		if (last_rev_color != rev_color) {
			last_rev_color = rev_color;
			controls.rev_led.set_base_color(rev_color);
		}
		if (last_bank_color != bank_color) {
			last_bank_color = bank_color;
			controls.bank_led.set_base_color(bank_color);
		}

		// Breathe / flash
		if (flags.take(Flag::StartupDone)) {
			controls.bank_led.reset_breathe();
		}

		// Sample Slot Change
		if (flags.take(Flag::PlaySampleChangedValid))
			controls.play_led.flash_once_ms(Colors::white, 20);
		if (flags.take(Flag::PlaySampleChangedValidBright))
			controls.play_led.flash_once_ms(Colors::white, 120);
		if (flags.take(Flag::PlaySampleChangedEmpty))
			controls.play_led.flash_once_ms(Colors::red, 20);
		if (flags.take(Flag::PlaySampleChangedEmptyBright))
			controls.play_led.flash_once_ms(Colors::red, 120);
	}

	void animate_startup() {
		// Startup sequence
		if (flags.take(Flag::StartupParsing))
			controls.rev_led.flash_once_ms(Colors::yellow, 100);
		if (flags.take(Flag::StartupLoadingIndex)) {
			controls.bank_led.set_base_color(Colors::orange);
			controls.bank_led.breathe(SamplerColors::Bank::purple, 1);
		}
		if (flags.take(Flag::StartupNewIndex)) {
			controls.bank_led.set_base_color(Colors::off);
			controls.bank_led.breathe(Colors::white, 1);
		}
		if (flags.take(Flag::StartupWritingIndex)) {
			controls.bank_led.set_base_color(Colors::off);
			controls.bank_led.breathe(Colors::magenta, 1);
		}
		if (flags.take(Flag::StartupWritingHTML)) {
			controls.bank_led.set_base_color(Colors::off);
			controls.bank_led.breathe(Colors::magenta, 0.5);
		}
	}
};
} // namespace SamplerKit
