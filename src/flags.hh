#pragma once
#include <cstdint>

namespace SamplerKit
{
struct Flags {
private:
	// TODO: make these atomic, if necessary
	bool _time_changed = true;
	bool _inf_changed = false;
	bool _rev_changed = false;
	bool _disable_mode_changes = false;
	float _scroll_loop_amt = 0.f;

public:
	uint32_t mute_on_boot_ctr = 12000;

	bool take_time_changed() {
		auto t = _time_changed;
		_time_changed = false;
		return t;
	}
	bool take_inf_changed() {
		if (_disable_mode_changes)
			return false;
		auto t = _inf_changed;
		_inf_changed = false;
		return t;
	}
	bool take_rev_changed() {
		if (_disable_mode_changes)
			return false;
		auto t = _rev_changed;
		_rev_changed = false;
		return t;
	}
	float take_scroll_amt() {
		auto amt = _scroll_loop_amt;
		_scroll_loop_amt = 0.f;
		return amt;
	}

	void set_time_changed() { _time_changed = true; }
	void set_inf_changed() { _inf_changed = true; }
	void set_rev_changed() { _rev_changed = true; }
	void disable_mode_changes() { _disable_mode_changes = true; }
	void enable_mode_changes() { _disable_mode_changes = false; }
	void set_scroll_amt(float amt) { _scroll_loop_amt = amt; }
	void add_scroll_amt(float amt) { _scroll_loop_amt += amt; }
};
} // namespace SamplerKit
