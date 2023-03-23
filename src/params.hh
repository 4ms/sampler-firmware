#pragma once
#include "audio_stream_conf.hh"
#include "bank.hh"
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
#include "util/colors.hh"
#include "util/countzip.hh"
#include "util/math.hh"

namespace SamplerKit
{
enum class PlayStates {
	SILENT,
	PREBUFFERING,
	PLAY_FADEUP,
	PERC_FADEUP,
	PLAYING,
	PLAYING_PERC,
	PLAY_FADEDOWN,
	RETRIG_FADEDOWN,
	REV_PERC_FADEDOWN,
	PAD_SILENCE,
};

// Params holds all the modes, settings and parameters for the sampler
// Params are set by controls (knobs, jacks, buttons, etc)
struct Params {
	Controls &controls;
	Flags &flags;
	CalibrationStorage &system_calibrations;
	UserSettings &settings;
	BankManager &banks;

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

	// global_mode[]:
	enum class MonitorModes { OFF = 0b00, BOTH = 0b11, LEFT = 0b01, RIGHT = 0b10 };
	MonitorModes monitor_recording = MonitorModes::OFF;
	bool enable_recording = false;
	// No: EDIT_MODE, ASSIGN_MODE, ALLOW_SPLIT_MONITORING, VIDEO_DIM

	// These are what's playing, even if the controls have selected something else
	uint8_t sample_num_now_playing;
	uint8_t sample_bank_now_playing;

	uint32_t play_trig_timestamp = 0;
	int32_t voct_latch_value = 0;

	PlayStates play_state = PlayStates::SILENT;
	OperationMode op_mode = OperationMode::Normal;

	Params(Controls &controls,
		   Flags &flags,
		   CalibrationStorage &system_calibrations,
		   UserSettings &settings,
		   BankManager &banks)
		: controls{controls}
		, flags{flags}
		, system_calibrations{system_calibrations}
		, settings{settings}
		, banks{banks} {

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

		if (op_mode == OperationMode::Calibrate) {
			// TODO: Calibrate mode
			//  update_calibration();
		}

		if (op_mode == OperationMode::SystemMode) {
			// TODO: System Settings mode
			//  update_system_settings();
		}

		// check_entering_system_mode();

		update_leds();
		update_button_modes();
	}

private:
	void update_pitch() {
		auto &pot = pot_state[PitchPot];
		auto potval = std::clamp(pot.cur_val + (int16_t)system_calibrations.pitch_pot_detent_offset, 0, 4095);

		// Flag::LatchVoltOctCV is set after a Play Trig happens
		// and the current CV value is stored into voct_latch_value.
		// After a delay, the latch flag is released and playback begins
		int16_t pitch_cv;
		if (flags.read(Flag::LatchVoltOctCV))
			pitch_cv = voct_latch_value;
		else
			pitch_cv = cv_state[PitchCV].cur_val;

		pitch_cv = MathTools::plateau<6, 2048>(4095 - pitch_cv);

		uint32_t compensated_pitch_cv =
			TuningCalcs::apply_tracking_compensation(pitch_cv, system_calibrations.tracking_comp);

		if (settings.quantize)
			pitch = pitch_pot_lut[potval] * TuningCalcs::quantized_semitone_voct(compensated_pitch_cv);
		else
			pitch = pitch_pot_lut[potval] + voltoct[compensated_pitch_cv];

		// pitch = std::min((potval + pitch_cv) / 4096.f, MAX_RS);
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
		if (start < 0.005f)
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
			if (diff > Board::MinPotChange)
				pot.track_moving_ctr = 250; // 250 for 6kHz = 42ms

			if (pot.track_moving_ctr) {
				pot.track_moving_ctr--;
				pot.prev_val = pot.cur_val;
				pot.delta = diff;
				// pot.moved = true;

				if (controls.rev_button.is_pressed()) {
					if (!pot.moved_while_rev_down)
						pot.latched_val = pot.cur_val;
					pot.moved_while_rev_down = true;
					ignore_rev_release = true;
				}

				if (controls.bank_button.is_pressed()) {
					if (!pot.moved_while_bank_down)
						pot.latched_val = pot.cur_val;
					pot.moved_while_bank_down = true;
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
				cv.cur_val += system_calibrations.cv_calibration_offset[i];

			int16_t diff = std::abs(cv.cur_val - cv.prev_val);
			if (diff > Board::MinCVChange) {
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

	void update_button_modes() {
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
		}

		if (controls.play_button.is_pressed()) {
			if (controls.play_button.how_long_held() == (Board::ParamUpdateHz / 2)) // 0.5 sec
			{
				flags.set(Flag::ToggleLooping);
				ignore_play_release = true;
			}
		}

		if (controls.bank_button.is_just_released()) {
			if (!ignore_bank_release) {
				if (controls.rev_button.is_pressed()) {
					bank = banks.prev_enabled_bank(bank);
					ignore_rev_release = true;
				} else
					bank = banks.next_enabled_bank(bank);
			}

			ignore_bank_release = false;
			for (auto &pot : pot_state)
				pot.moved_while_bank_down = false;
		}

		// STS: TODO: long-hold Reverse with Length at extreme toggles env modes
		if (controls.rev_button.is_just_released()) {
			if (!ignore_rev_release) {
				flags.set(Flag::RevTrig);
			}

			ignore_rev_release = false;
			for (auto &pot : pot_state) {
				if (pot.moved_while_rev_down)
					pot.is_catching_up = true;
				pot.moved_while_rev_down = false;
			}
		}
	}

	void update_leds() {

		Color rev_color;
		rev_color = reverse ? Colors::blue : Colors::off;

		Color play_color;
		if (play_state != PlayStates::SILENT && play_state != PlayStates::PREBUFFERING) {
			play_color = looping ? SamplerColors::cyan : SamplerColors::green;
		} else
			play_color = Colors::off;

		Color bank_color;
		bank_color = (BankColors[bank % 10]);
		// TODO: flash bank LED bank /10 times per second

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

		if (flags.take(Flag::PlaySampleChangedValid))
			controls.play_led.flash_once_ms(Colors::white, 20);
		if (flags.take(Flag::PlaySampleChangedValidBright))
			controls.play_led.flash_once_ms(Colors::white, 120);

		if (flags.take(Flag::PlaySampleChangedEmpty))
			controls.play_led.flash_once_ms(Colors::red, 20);
		if (flags.take(Flag::PlaySampleChangedEmptyBright))
			controls.play_led.flash_once_ms(Colors::red, 120);

		if (flags.take(Flag::StartupLoadingIndex))
			controls.bank_led.breathe(Colors::orange, 1);

		if (flags.take(Flag::StartupNewIndex))
			controls.bank_led.breathe(Colors::white, 1);

		if (flags.take(Flag::StartupWritingIndex))
			controls.bank_led.breathe(Colors::magenta, 1);

		if (flags.take(Flag::StartupWritingHTML))
			controls.bank_led.breathe(Colors::purple, 1);

		if (flags.take(Flag::StartupDone))
			controls.bank_led.reset_breathe();
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
	std::array<CVState, NumPots> cv_state;

	uint32_t end_out_ctr = 0;

	bool ignore_bank_release = false;
	bool ignore_rev_release = false;
	bool ignore_play_release = false;

	Color last_play_color = Colors::off;
	Color last_rev_color = Colors::off;
	Color last_bank_color = Colors::off;
};

constexpr auto ParamsSize = sizeof(Params);

} // namespace SamplerKit
