#pragma once
#include "controls.hh"
#include "flags.hh"
#include "log.hh"
#include "pot_state.hh"
#include "settings.hh"
#include "util/voct_calibrator.hh"

namespace SamplerKit
{

struct ButtonActionHandler {
	Flags &flags;
	Controls &controls;
	std::array<PotState, NumPots> &pot_state;

	bool ignore_bank_release = false;
	bool ignore_rev_release = false;
	bool ignore_play_release = false;

	static constexpr uint32_t short_press = Brain::ParamUpdateHz * 0.3f;
	static constexpr uint32_t half_sec = Brain::ParamUpdateHz * 0.5f;
	static constexpr uint32_t one_sec = Brain::ParamUpdateHz;
	static constexpr uint32_t two_sec = Brain::ParamUpdateHz * 2.f;

	ButtonActionHandler(Flags &flags, Controls &controls, std::array<PotState, NumPots> &pot_state)
		: flags{flags}
		, controls{controls}
		, pot_state{pot_state} {}

	// TODO: put all button ops in process() and check in each button combo for op_mode
	void process(OperationMode op_mode, bool looping) {
		switch (op_mode) {
			case OperationMode::CVCalibrate:
				process_cvcal_mode();
				break;

			case OperationMode::SystemMode:
				break;

			case OperationMode::Playback:
				process_play_mode(looping);
				break;

			case OperationMode::Record:
				process_rec_mode();
				break;
		}
	}

	void process_play_mode(bool looping) {
		if (controls.play_button.is_just_pressed()) {
			if (!looping)
				flags.set(Flag::PlayBut);
		}

		if (controls.play_button.is_just_released()) {
			if (looping && !ignore_play_release) {
				flags.set(Flag::PlayBut);
			}

			ignore_play_release = false;
		}

		// Long hold Play and Rev to toggle Rec/Play mode
		if (!ignore_rev_release && controls.rev_button.how_long_held_pressed() > one_sec) {
			if (!ignore_play_release && controls.play_button.how_long_held_pressed() > one_sec) {
				if (!controls.bank_button.is_pressed()) {
					flags.set(Flag::EnterRecordMode);
					ignore_play_release = true;
					ignore_rev_release = true;
				}
			}
		}

		// Long hold Play to toggle looping
		if (!ignore_play_release && controls.play_button.how_long_held_pressed() > short_press) {
			if (!controls.rev_button.is_pressed() && !controls.bank_button.is_pressed()) {
				flags.set(Flag::ToggleLooping);
				ignore_play_release = true;
			}
		}

		// Long hold Bank + Rev for CV Calibration
		if (!ignore_bank_release && controls.bank_button.how_long_held_pressed() > half_sec) {
			if (!ignore_rev_release && controls.rev_button.how_long_held_pressed() > half_sec) {
				if (!controls.play_button.is_pressed()) {
					flags.set(Flag::EnterCVCalibrateMode);
					ignore_bank_release = true;
					ignore_rev_release = true;
				}
			}
		}

		// Long hold Play + Bank + Rev to save index
		if (!ignore_bank_release && controls.bank_button.how_long_held_pressed() > two_sec) {
			if (!ignore_rev_release && controls.rev_button.how_long_held_pressed() > two_sec) {
				if (!ignore_play_release && controls.play_button.how_long_held_pressed() > two_sec) {
					flags.set(Flag::WriteIndexToSD);
					// flags.set(Flag::WriteSettingsToSD);
					ignore_bank_release = true;
					ignore_play_release = true;
					ignore_rev_release = true;
				}
			}
		}

		// Bank -> change bank
		if (controls.bank_button.is_just_released()) {
			if (!ignore_bank_release) {
				if (controls.rev_button.is_pressed()) {
					flags.set(Flag::BankPrevEnabled);
					ignore_rev_release = true;
				} else {
					flags.set(Flag::BankNextEnabled);
				}
			}

			for (auto &pot : pot_state)
				pot.moved_while_bank_down = false;

			ignore_bank_release = false;
		}

		// Reverse -> toggle
		if (controls.rev_button.is_just_released()) {
			if (!ignore_rev_release) {
				flags.set(Flag::RevTrig);
			}

			for (auto &pot : pot_state) {
				if (pot.moved_while_rev_down)
					pot.is_catching_up = true;
				pot.moved_while_rev_down = false;
			}

			ignore_rev_release = false;
		}
	}

	void process_rec_mode() {
		if (controls.play_button.is_just_pressed()) {
		}

		if (controls.play_button.is_just_released()) {
			if (!ignore_play_release)
				flags.set(Flag::RecBut);
			ignore_play_release = false;
		}

		// Long hold Play and Rev to toggle Rec/Play mode
		if (!ignore_rev_release && controls.rev_button.how_long_held_pressed() > one_sec) {
			if (!ignore_play_release && controls.play_button.how_long_held_pressed() > one_sec) {
				controls.play_led.reset_breathe();
				flags.set(Flag::EnterPlayMode);
				ignore_play_release = true;
				ignore_rev_release = true;
			}
		}

		if (controls.bank_button.is_just_released()) {
			if (!ignore_bank_release) {
				if (controls.rev_button.is_pressed()) {
					flags.set(Flag::BankPrevEnabled);
					ignore_rev_release = true;
				} else {
					flags.set(Flag::BankNextEnabled);
				}
			}
			ignore_bank_release = false;
		}

		if (controls.rev_button.is_just_released()) {
			ignore_rev_release = false;
		}
	}

	void process_cvcal_mode() {
		// Long hold Bank + Rev to exit CV Calibration
		if (!ignore_bank_release && controls.bank_button.how_long_held_pressed() > two_sec) {
			if (!ignore_rev_release && controls.rev_button.how_long_held_pressed() > two_sec) {
				if (!controls.play_button.is_pressed()) {
					flags.set(Flag::EnterPlayMode);
					ignore_bank_release = true;
					ignore_rev_release = true;
				}
			}
		}

		if (controls.rev_button.is_just_released()) {
			ignore_rev_release = false;
		}

		if (controls.play_button.is_just_released()) {
			flags.set(Flag::StepCVCalibration);
			ignore_play_release = false;
		}

		if (controls.bank_button.is_just_released()) {
			ignore_bank_release = false;
		}
	}

private:
};

} // namespace SamplerKit
