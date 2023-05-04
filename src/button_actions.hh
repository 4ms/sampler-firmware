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
	bool ignore_bank_longhold = false;
	bool ignore_rev_longhold = false;
	bool ignore_play_longhold = false;

	static constexpr uint32_t short_press = Brain::ParamUpdateHz * 0.3f;
	static constexpr uint32_t half_sec = Brain::ParamUpdateHz * 0.5f;
	static constexpr uint32_t one_sec = Brain::ParamUpdateHz;
	static constexpr uint32_t two_sec = Brain::ParamUpdateHz * 2.f;

	ButtonActionHandler(Flags &flags, Controls &controls, std::array<PotState, NumPots> &pot_state)
		: flags{flags}
		, controls{controls}
		, pot_state{pot_state} {}

	// TODO: put all button ops in process() and check in each button combo for op_mode
	// Combine ignore_*_release and ignore_*_longhold: they are always set to the same value
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
			ignore_play_longhold = false;
		}

		// Long hold Play and Rev to toggle Rec/Play mode
		if (!ignore_rev_longhold && controls.rev_button.how_long_held_pressed() > one_sec) {
			if (!ignore_play_longhold && controls.play_button.how_long_held_pressed() > one_sec) {
				if (!controls.bank_button.is_pressed()) {
					flags.set(Flag::EnterRecordMode);
					ignore_play_longhold = true;
					ignore_rev_longhold = true;
					ignore_play_release = true;
					ignore_rev_release = true;
				}
			}
		}

		// Long hold Play to toggle looping
		if (!ignore_play_longhold && controls.play_button.how_long_held_pressed() > short_press) {
			if (!controls.rev_button.is_pressed() && !controls.bank_button.is_pressed()) {
				flags.set(Flag::ToggleLooping);
				ignore_play_longhold = true;
				ignore_play_release = true;
			}
		}

		// Long hold Bank + Rev for CV Calibration
		if (!ignore_bank_longhold && controls.bank_button.how_long_held_pressed() > half_sec) {
			if (!ignore_rev_longhold && controls.rev_button.how_long_held_pressed() > half_sec) {
				if (!controls.play_button.is_pressed()) {
					// calibration.cv_calibration_offset[0] = bank;
					// cal_storage.save_flash_params();
					flags.set(Flag::EnterCVCalibrateMode);
					ignore_bank_release = true;
					ignore_rev_release = true;
					ignore_bank_longhold = true;
					ignore_rev_longhold = true;
				}
			}
		}

		// Long hold Play + Bank + Rev to save index
		if (!ignore_bank_longhold && controls.bank_button.how_long_held_pressed() > half_sec) {
			if (!ignore_rev_longhold && controls.rev_button.how_long_held_pressed() > half_sec) {
				if (!ignore_play_longhold && controls.play_button.how_long_held_pressed() > half_sec) {
					flags.set(Flag::WriteIndexToSD);
					ignore_bank_release = true;
					ignore_play_release = true;
					ignore_rev_release = true;
					ignore_bank_longhold = true;
					ignore_play_longhold = true;
					ignore_rev_longhold = true;
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

			ignore_bank_longhold = false;
			ignore_bank_release = false;
		}

		// STS: TODO: long-hold Reverse with Length at extreme toggles env modes
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
			ignore_rev_longhold = false;
		}
	}

	void process_rec_mode() {
		if (controls.play_button.is_just_pressed()) {
		}

		if (controls.play_button.is_just_released()) {
			if (!ignore_play_release)
				flags.set(Flag::RecBut);
			ignore_play_longhold = false;
			ignore_play_release = false;
		}

		// Long hold Play and Rev to toggle Rec/Play mode
		if (!ignore_rev_longhold && controls.rev_button.how_long_held_pressed() > one_sec) {
			if (!ignore_play_longhold && controls.play_button.how_long_held_pressed() > one_sec) {
				controls.play_led.reset_breathe();
				flags.set(Flag::EnterPlayMode);
				ignore_play_longhold = true;
				ignore_rev_longhold = true;
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
			ignore_bank_longhold = false;
			ignore_bank_release = false;
		}

		if (controls.rev_button.is_just_released()) {
			ignore_rev_longhold = false;
			ignore_rev_release = false;
		}
	}

	void process_cvcal_mode() {
		// Long hold Bank + Rev to exit CV Calibration
		if (!ignore_bank_longhold && controls.bank_button.how_long_held_pressed() > two_sec) {
			if (!ignore_rev_longhold && controls.rev_button.how_long_held_pressed() > two_sec) {
				if (!controls.play_button.is_pressed()) {
					flags.set(Flag::EnterPlayMode);
					ignore_bank_release = true;
					ignore_rev_release = true;
					ignore_bank_longhold = true;
					ignore_rev_longhold = true;
				}
			}
		}

		if (controls.rev_button.is_just_released()) {
			ignore_rev_longhold = false;
			ignore_rev_release = false;
		}

		if (controls.play_button.is_just_released()) {
			flags.set(Flag::StepCVCalibration);
			ignore_play_longhold = false;
			ignore_play_release = false;
		}

		if (controls.bank_button.is_just_released()) {
			ignore_bank_longhold = false;
			ignore_bank_release = false;
		}
	}

private:
};

} // namespace SamplerKit
