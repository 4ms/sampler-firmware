#pragma once
#include "audio_stream_conf.hh"
#include <cstdint>

enum class GateType { Gate, Trig };
enum class InfState { Off, On, TransitioningOn, TransitioningOff };

struct ChannelMode {
	InfState inf = InfState::Off;
	bool reverse = false;
	bool time_pot_quantized = true;
	bool time_cv_quantized = true;
	bool ping_locked = false;
	bool quantize_mode_changes = true;
	bool adjust_loop_end = false; // flag_pot_changed_revdown[TIME]
};

// Settings cannot be changed in Normal operation mode
struct Settings {
	GateType rev_jack = GateType::Trig;
	GateType endout_jack = GateType::Trig;
	GateType start_jack = GateType::Trig;
	GateType main_clock = GateType::Gate;
	bool log_delay_feed = true;
	bool runaway_dc_block = true;
	bool auto_unquantize_timejack = true;
	bool send_return_before_loop = false;
	uint32_t led_brightness = 4;
};

enum class OperationMode { Normal, SysSettings, Calibrate };
