#pragma once
#include "audio_stream_conf.hh"
#include "bank.hh"
#include "bank_blink.hh"
#include "calibration_storage.hh"
#include "controls.hh"
#include "elements.hh"
#include "flags.hh"
#include "lut/pitch_pot_lut.h"
#include "lut/voltoct.h"
#include "palette.hh"
#include "sample_file.hh"
#include "sample_pot_detents.hh"
#include "settings.hh"
#include "timing_calcs.hh"
#include "tuning_calcs.hh"
#include "ui.hh"
#include "util/colors.hh"
#include "util/countzip.hh"
#include "util/math.hh"

namespace SamplerKit
{

// Params holds all the modes, settings and parameters for the sampler
// Params are set by controls (knobs, jacks, buttons, etc)
struct Params {

	Controls &controls;
	Flags &flags;
	UserSettings &settings;
	BankManager &banks;

	CalibrationStorage cal_storage;
	CalibrationData &calibration;

	Ui ui{flags, controls};

	// i_param[]:
	uint32_t bank = 0;
	uint32_t sample = 0;
	bool reverse = 0;
	bool looping = 0;

	// f_param[]:
	float pitch = 1.0f;
	float start = 0.f;
	float length = 1.f;
	float volume = 1.f;

	// These are what's playing, even if the controls have selected something else
	uint8_t sample_num_now_playing = 0;
	uint8_t sample_bank_now_playing = 0;

	uint32_t play_trig_timestamp = 0;
	int32_t voct_latch_value = 0;

	uint32_t bank_button_sel = 0;

	PlayStates play_state = PlayStates::SILENT;
	RecStates rec_state = RecStates::REC_OFF;

	OperationMode op_mode = OperationMode::Playback;

	Params(Controls &controls, Flags &flags, UserSettings &settings, BankManager &banks)
		: controls{controls}
		, flags{flags}
		, settings{settings}
		, banks{banks}
		, calibration{cal_storage.cal_data} {

		controls.start();
	}

	void update() {
		controls.update();

		update_endout_jack();
		update_trig_jacks();

		update_pot_states();
		update_cv_states();

		update_length();
		update_startpos();
		update_sample();
		update_pitch();
		update_bank_cv();

		ui.update_leds({op_mode, play_state, rec_state, reverse, looping, bank});
		// check_entering_system_mode();

		if (op_mode == OperationMode::Calibrate) {
			// TODO: Calibrate mode
			//  update_calibration();
		}

		if (op_mode == OperationMode::SystemMode) {
			// TODO: System Settings mode
			//  update_system_settings();
		}

		if (op_mode == OperationMode::Playback) {
			update_play_mode_buttons();
		} else if (op_mode == OperationMode::Record) {
			update_rec_mode_buttons();
		}
	}

	void startup_animation() {
		controls.update();
		ui.animate_startup();
	}

private:
	void update_pitch() {
		auto &pot = pot_state[PitchPot];
		auto potval = std::clamp(pot.cur_val + (int16_t)calibration.pitch_pot_detent_offset, 0, 4095);

		// Flag::LatchVoltOctCV is set after a Play Trig happens
		// and the current CV value is stored into voct_latch_value.
		// After a delay, the latch flag is released and playback begins
		int16_t pitch_cv;
		if (flags.read(Flag::LatchVoltOctCV))
			pitch_cv = voct_latch_value;
		else
			pitch_cv = cv_state[PitchCV].cur_val;

		pitch_cv = MathTools::plateau<12, 2048>(pitch_cv) + 2048;

		uint32_t compensated_pitch_cv = TuningCalcs::apply_tracking_compensation(pitch_cv, calibration.tracking_comp);

		if (settings.quantize)
			pitch = pitch_pot_lut[potval] * TuningCalcs::quantized_semitone_voct(compensated_pitch_cv);
		else
			pitch = pitch_pot_lut[potval] * voltoct[compensated_pitch_cv];
	}

	void update_length() {
		auto &pot = pot_state[LengthPot];
		float potval;
		if (pot.moved_while_rev_down) {
			potval = pot.latched_val;
			// TODO: STS: Edit+Length = fade time
			// fade_time = pot.cur_val / 4095.f;
		} else
			potval = pot.cur_val;

		length = (potval + cv_state[LengthCV].cur_val) / 4096.f;
		if (length < 0.005f)
			length = 0.005f;
		if (length > 0.990f)
			length = 1.f;
	}

	void update_startpos() {
		auto &pot = pot_state[StartPot];

		float pot_start;

		if (pot.moved_while_rev_down) {
			// Rev + StartPos = Volume
			pot_start = pot.latched_val;
			// TODO: use value-crossing to update volume so vol doesnt jump when we start the push+turn
			// if (std::abs(pot.cur_val / 4095.f - volume) < 0.04f)
			volume = pot.cur_val / 4095.f;
		} else {
			if (pot.is_catching_up && pot.latched_val == pot.cur_val)
				pot.is_catching_up = false;
			pot_start = pot.is_catching_up ? pot.latched_val : pot.cur_val;
		}

		start = (pot_start + cv_state[StartCV].cur_val) / 4096.f;
		if (start < 0.01f)
			start = 0.f;
		if (start > 0.99f)
			start = 1.f;
	}

	void update_sample() {
		auto &pot = pot_state[SamplePot];
		float potval;
		if (pot.moved_while_bank_down) {
			potval = pot.latched_val;
			// STS: TODO: Bank + Sample changes bank
			//  bank_hover = detent_num(pot.cur_val); ...etc
		} else
			potval = pot.cur_val;

		auto new_sample = detent_num_antihys(potval + cv_state[SampleCV].cur_val, sample);
		if (new_sample != sample) {
			sample = new_sample;
			flags.set(Flag::PlaySampleChanged);
		}
	}

	void update_pot_states() {
		for (auto [i, pot] : enumerate(pot_state)) {
			pot.cur_val = (int16_t)controls.read_pot(static_cast<PotAdcElement>(i));

			int16_t diff = std::abs(pot.cur_val - pot.prev_val);
			if (diff > Brain::MinPotChange) {
				pot.track_moving_ctr = 250; // 250 for 6kHz = 42ms
			}

			if (pot.track_moving_ctr) {
				pot.track_moving_ctr--;
				pot.prev_val = pot.cur_val;
				pot.delta = diff;
				// pot.moved = true;

				if (controls.rev_button.is_pressed()) {
					if (!pot.moved_while_rev_down)
						pot.latched_val = pot.cur_val;
					pot.moved_while_rev_down = true;
					if (i == StartPot)
						ignore_rev_release = true;
				}

				if (controls.bank_button.is_pressed()) {
					if (!pot.moved_while_bank_down)
						pot.latched_val = pot.cur_val;
					pot.moved_while_bank_down = true;
					// Allow Sample + Bank since the combo does nothing
					if (i != SamplePot)
						ignore_bank_release = true;
				}
			}
		}
	}

	void update_cv_states() {
		for (auto [i, cv] : enumerate(cv_state)) {
			cv.cur_val = (int16_t)controls.read_cv(static_cast<CVAdcElement>(i));
			if (op_mode == OperationMode::Calibrate) {
				// TODO: use raw values, without calibration offset
			} else
				cv.cur_val += calibration.cv_calibration_offset[i];

			int16_t diff = std::abs(cv.cur_val - cv.prev_val);
			if (diff > Brain::MinCVChange) {
				cv.delta = diff;
				cv.prev_val = cv.cur_val;
			}
		}
	}

	void update_trig_jacks() {
		if (controls.play_jack.is_just_pressed()) {
			if (play_state == PlayStates::PLAYING)
				play_state = PlayStates::PLAY_FADEDOWN;

			voct_latch_value = cv_state[PitchCV].cur_val;
			play_trig_timestamp = HAL_GetTick();
			flags.set(Flag::PlayTrigDelaying);
		}

		if (controls.rev_jack.is_just_pressed()) {
			flags.set(Flag::RevTrig);
		}
	}

	void update_endout_jack() {
		if (flags.take(Flag::EndOutShort))
			end_out_ctr = 8;

		if (flags.take(Flag::EndOutLong))
			end_out_ctr = 35;

		if (end_out_ctr == 1) {
			controls.end_out.low();
			end_out_ctr = 0;
		} else if (end_out_ctr > 1) {
			controls.end_out.high();
			end_out_ctr--;
		}
	}

	void update_play_mode_buttons() {
		// if (flags.take(Flag::SkipProcessButtons)) return;

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
		if (!ignore_rev_longhold && controls.rev_button.how_long_held_pressed() > (Brain::ParamUpdateHz)) {
			if (!ignore_play_longhold && controls.play_button.how_long_held_pressed() > (Brain::ParamUpdateHz)) {
				if (!controls.bank_button.is_pressed()) {
					op_mode = OperationMode::Record;
					looping = false;
					flags.set(Flag::EnterRecordMode);
					ignore_play_longhold = true;
					ignore_rev_longhold = true;
					ignore_play_release = true;
					ignore_rev_release = true;
				}
			}
		}

		// Long hold Play to toggle looping
		if (!ignore_play_longhold && controls.play_button.how_long_held_pressed() > (Brain::ParamUpdateHz * 0.3f)) {
			if (!controls.rev_button.is_pressed() && !controls.bank_button.is_pressed()) {
				flags.set(Flag::ToggleLooping);
				ignore_play_longhold = true;
				ignore_play_release = true;
			}
		}

		// Long hold Bank + Rev for CV Calibration
		if (!ignore_bank_longhold && controls.bank_button.how_long_held_pressed() > (Brain::ParamUpdateHz * 0.5f)) {
			if (!ignore_rev_longhold && controls.rev_button.how_long_held_pressed() > (Brain::ParamUpdateHz * 0.5f)) {
				if (!controls.play_button.is_pressed()) {
					// calibration.cv_calibration_offset[0] = bank;
					// cal_storage.save_flash_params();
					printf_("CV Cal %d\n", bank);
					ignore_bank_release = true;
					ignore_rev_release = true;
					ignore_bank_longhold = true;
					ignore_rev_longhold = true;
				}
			}
		}

		// Long hold Play + Bank + Rev for LED Calibration
		if (!ignore_bank_longhold && controls.bank_button.how_long_held_pressed() > (Brain::ParamUpdateHz * 0.5f)) {
			if (!ignore_rev_longhold && controls.rev_button.how_long_held_pressed() > (Brain::ParamUpdateHz * 0.5f)) {
				if (!ignore_play_longhold &&
					controls.play_button.how_long_held_pressed() > (Brain::ParamUpdateHz * 0.5f))
				{
					// calibration.cv_calibration_offset[0] = bank;
					// cal_storage.save_flash_params();
					printf_("LED Cal %d\n", bank);
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
					bank_button_sel = banks.prev_enabled_bank(bank_button_sel);
					ignore_rev_release = true;
				} else {
					bank_button_sel = banks.next_enabled_bank(bank_button_sel);
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

	void update_rec_mode_buttons() {
		if (controls.play_button.is_just_pressed()) {
		}

		if (controls.play_button.is_just_released()) {
			if (!ignore_play_release)
				flags.set(Flag::RecBut);
			ignore_play_longhold = false;
			ignore_play_release = false;
		}

		// Long hold Play and Rev to toggle Rec/Play mode
		if (!ignore_rev_longhold && controls.rev_button.how_long_held_pressed() > (Brain::ParamUpdateHz)) {
			if (!ignore_play_longhold && controls.play_button.how_long_held_pressed() > (Brain::ParamUpdateHz)) {

				controls.play_led.reset_breathe();
				flags.set(Flag::EnterPlayMode);
				op_mode = OperationMode::Playback;
				ignore_play_longhold = true;
				ignore_rev_longhold = true;
				ignore_play_release = true;
				ignore_rev_release = true;
			}
		}

		if (controls.bank_button.is_just_released()) {
			if (!ignore_bank_release) {
				if (controls.rev_button.is_pressed()) {
					bank_button_sel = banks.prev_enabled_bank(bank_button_sel);
					ignore_rev_release = true;
				} else {
					bank_button_sel = banks.next_enabled_bank(bank_button_sel);
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

	void update_bank_cv() {
		// Final bank is closest enabled bank to (button_bank_i + bank_cv_i)
		float bank_cv = (float)cv_state[BankCV].cur_val / 4096.f * 60.f; // 0..4096 => 0..60

		// Short-circuit if no or low CV:
		if (bank_cv < 0.5f)
			bank = bank_button_sel;

		// TODO: actual anti-hysteresis
		static float last_bank_cv = 0.f;
		if (std::abs(last_bank_cv - bank_cv) < 0.25f)
			return;
		last_bank_cv = bank_cv;

		int32_t t_bank = (uint32_t)(bank_cv + 0.5f) + bank_button_sel;
		static int32_t last_t_bank = 0xFFFFFFFF;
		if (last_t_bank == t_bank)
			return;
		last_t_bank = t_bank;

		if (t_bank >= 60)
			t_bank = t_bank % 60;

		if (banks.is_bank_enabled(t_bank)) {
			bank = t_bank;
			return;
		}

		int32_t last_bank = banks.prev_enabled_bank(0);
		if (t_bank > last_bank)
			t_bank = t_bank % (last_bank + 1);

		if (banks.is_bank_enabled(t_bank)) {
			bank = t_bank;
			return;
		}

		// Pick nearest of next or prev enabled bank, without wrapping
		int32_t next = banks.next_enabled_bank(t_bank);
		int32_t prev = banks.prev_enabled_bank(t_bank);
		int32_t next_diff = std::abs(next - t_bank);
		int32_t prev_diff = std::abs(t_bank - prev);
		bank = (next_diff < prev_diff) ? next : prev;
	}

private:
	struct PotState {
		int16_t cur_val = 0;
		int16_t prev_val = 0;		  // old_i_smoothed_potadc
		int16_t track_moving_ctr = 0; // track_moving_pot
		int16_t delta = 0;			  // pot_delta
		int16_t latched_val = 0;
		bool is_catching_up = false;
		bool moved_while_bank_down = false; // flag_pot_changed_infdown
		bool moved_while_rev_down = false;	// flag_pot_changed_revdown
											// bool moved = false;					// flag_pot_changed
	};
	std::array<PotState, NumPots> pot_state;

	struct CVState {
		int16_t cur_val = 0;
		int16_t prev_val = 0;
		int16_t delta = 0;
	};
	std::array<CVState, NumCVs> cv_state;

	uint32_t end_out_ctr = 0;

	bool ignore_bank_release = false;
	bool ignore_rev_release = false;
	bool ignore_play_release = false;

	bool ignore_bank_longhold = false;
	bool ignore_rev_longhold = false;
	bool ignore_play_longhold = false;
};

constexpr auto ParamsSize = sizeof(Params);

} // namespace SamplerKit
