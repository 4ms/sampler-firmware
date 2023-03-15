#pragma once
#include <cstdint>

enum class GateType { Gate, Trig };
enum class AutoStopMode { Off = 0, Always = 1, Looping = 2 };

// Was global_modes[...] and global_params.
struct UserSettings {
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

	GateType rev_jack = GateType::Trig;
	GateType endout_jack = GateType::Trig;
	GateType start_jack = GateType::Trig;
	GateType main_clock = GateType::Gate;

	bool rec_24bits = false;
	uint32_t record_sample_rate = 48000;
};

enum class OperationMode { Normal, SysSettings, Calibrate };
