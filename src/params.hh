#pragma once
#include "audio_stream_conf.hh"
#include "controls.hh"
#include "epp_lut.hh"
#include "flags.hh"
#include "log_taper_lut.hh"
#include "settings.hh"
#include "util/countzip.hh"
#include "util/math.hh"

namespace SamplerKit
{
// Params holds all the modes, settings and parameters for the sampler delay
// Params are set by controls (knobs, jacks, buttons, etc)
struct Params {
	Controls &controls;
	Flags &flags;

	uint32_t bank = 0;
	uint32_t sample = 0;
	bool reverse = 0;
	bool looping = 0;
	float pitch = 1.0f;
	float start = 0.f;
	float length = 1.f;
	float volume = 1.f;

	UserSettings settings;
	OperationMode op_mode = OperationMode::Normal;

	Params(Controls &controls, Flags &flags)
		: controls{controls}
		, flags{flags} {}

	void update() {
		controls.update();

		update_pot_states();
		update_cv_states();

		// calc_delay_feed();
		// calc_feedback();
		// calc_time();
		// calc_mix();

		if (op_mode == OperationMode::Calibrate) {
			// TODO: Calibrate mode
			//  update_calibration();
		}

		if (op_mode == OperationMode::SysSettings) {
			// TODO: System Settings mode
			//  update_system_settings();
		}

		// check_entering_system_mode();

		update_leds();
		update_button_modes();
	}

private:
	void update_pot_states() {
		for (auto [i, pot] : enumerate(pot_state)) {
			pot.cur_val = (int16_t)controls.read_pot(static_cast<PotAdcElement>(i++));

			int16_t diff = std::abs(pot.cur_val - pot.prev_val);
			if (diff > Board::MinPotChange)
				pot.track_moving_ctr = 250;

			if (pot.track_moving_ctr) {
				pot.track_moving_ctr--;
				pot.prev_val = pot.cur_val;
				pot.delta = diff;
				pot.moved = true;

				if (controls.rev_button.is_pressed()) {
					pot.moved_while_rev_down = true;
					ignore_rev_release = true; // if i==TimePot and in InfMode only?
				}

				if (controls.bank_button.is_pressed()) {
					pot.moved_while_bank_down = true;
					ignore_bank_release = true; // if i==TimePot || FeedbackPot in InfMode only?
				}
			}
		}
	}

	void update_cv_states() {
		for (auto [i, cv] : enumerate(cv_state)) {
			cv.cur_val = (int16_t)controls.read_cv(static_cast<CVAdcElement>(i++));
			if (op_mode == OperationMode::Calibrate) {
				// TODO: use raw values, without calibration offset
			}
			int16_t diff = std::abs(cv.cur_val - cv.prev_val);
			if (diff > Board::MinCVChange) {
				cv.delta = diff;
				cv.prev_val = cv.cur_val;
			}
		}
	}

	void update_button_modes() {
		if (controls.play_button.just_went_low()) {
		}

		if (controls.bank_button.is_just_released()) {
			if (!ignore_bank_release) {
				// TODO: handle change bank
			}

			ignore_bank_release = false;
			for (auto &pot : pot_state)
				pot.moved_while_bank_down = false;
		}

		if (controls.rev_button.is_just_released()) {
			if (!ignore_rev_release) {
				// TODO:
				//  flags.set_rev_changed();
			}

			ignore_rev_release = false;
			for (auto &pot : pot_state)
				pot.moved_while_rev_down = false;
		}
	}

	void update_leds() {

		static bool last_rev = false;
		if (last_rev != reverse) {
			last_rev = reverse;
			controls.rev_led.set_color(reverse ? Colors::blue : Colors::off);
		}

		// TODO: Play LED

		// TODO: Bank LED
	}

private:
	struct PotState {
		int16_t cur_val = 0;
		int16_t prev_val = 0;				// old_i_smoothed_potadc
		int16_t track_moving_ctr = 0;		// track_moving_pot
		int16_t delta = 0;					// pot_delta
		bool moved_while_bank_down = false; // flag_pot_changed_infdown
		bool moved_while_rev_down = false;	// flag_pot_changed_revdown
		bool moved = false;					// flag_pot_changed
	};
	std::array<PotState, NumPots> pot_state;

	struct CVState {
		int16_t cur_val = 0;
		int16_t prev_val = 0;
		int16_t delta = 0;
	};
	std::array<CVState, NumPots> cv_state;

	bool ignore_bank_release = false;
	bool ignore_rev_release = false;
};

constexpr auto ParamsSize = sizeof(Params);

} // namespace SamplerKit
