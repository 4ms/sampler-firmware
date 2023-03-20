#pragma once
#include "audio_stream_conf.hh"
#include "timing_calcs.hh"
#include <cstdint>

namespace SamplerKit
{
enum class GateType { Gate, Trig };
enum class AutoStopMode { Off = 0, Always = 1, Looping = 2 };

// Was global_modes[...] and global_params.
struct UserSettings {
	// These are stored on SD Card
	// And changed with button-combos or in system mode
	bool stereo_mode = false;
	AutoStopMode auto_stop_on_sample_change = AutoStopMode::Off;
	bool length_full_start_stop;
	bool quantize = false;
	bool perc_env = true;
	bool fadeupdown_env = true;
	uint32_t startup_bank = 0;
	uint32_t trig_delay = 8;
	uint32_t fade_time_ms = 24;
	bool auto_inc_slot_num_after_rec_trig = false;
	bool rec_24bits = false;
	uint32_t record_sample_rate = 48000;

	// These are changed in system mode and stored... in flash?
	GateType rev_jack = GateType::Trig;
	GateType endout_jack = GateType::Trig;
	GateType start_jack = GateType::Trig;
	GateType main_clock = GateType::Gate;

	// calculated values (formerly in global_params)
	// Might move them to Sampler class?
	float play_trig_delay;
	float play_trig_latch_pitch_time;
	float fade_down_rate;
	float fade_up_rate;

	void update_timing_calcs() {
		play_trig_delay = TimingCalcs::calc_trig_delay(trig_delay);
		play_trig_latch_pitch_time = TimingCalcs::calc_pitch_latch_time(trig_delay);
		// FIXME: Check what BlockSize should be
		// STS uses HT16_CHAN_BUFF_LEN which is 16 (FramesPerBlock)
		fade_up_rate =
			TimingCalcs::calc_fade_updown_rate(record_sample_rate, AudioStreamConf::BlockSize / 2., fade_time_ms);
		fade_down_rate = fade_up_rate;
	}
};

// global_modes[SYSTEM_MODE]
enum class OperationMode { Normal, SystemMode, Calibrate };
} // namespace SamplerKit