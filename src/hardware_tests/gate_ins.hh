#pragma once
#include "conf/board_conf.hh"
#include "hardware_tests/util.hh"
#include "libhwtests/GateInChecker.hh"

namespace SamplerKit::HWTests
{

// Patch EndOut to Play, then Rev
struct TestGateIns : IGateInChecker {
	TestGateIns()
		: IGateInChecker{2} {}

	bool read_gate(uint8_t gate_num) override {
		if (gate_num == 0)
			return Board::PlayJack::PinT::read();
		if (gate_num == 1)
			return Board::RevJack::PinT::read();
		return false;
	}

	void set_test_signal(bool newstate) override {
		Board::EndOut::set(newstate);
		HAL_Delay(1);
	}

	void set_indicator(uint8_t indicate_num, bool newstate) override {
		if (indicate_num == 0)
			Board::PlayG::set(newstate);
		if (indicate_num == 1)
			Board::RevG::set(newstate);
	}

	void set_error_indicator(uint8_t channel, ErrorType err) override {
		if (channel == 0)
			Board::PlayR::set(err != ErrorType::None);
		if (channel == 1)
			Board::RevR::set(err != ErrorType::None);
	}

	void signal_jack_done(uint8_t chan) override {
		if (chan == 0)
			Board::PlayB::set(true);
		if (chan == 1)
			Board::BankG::set(true); //=>RevB
	}

	bool is_ready_to_read_jack(uint8_t chan) override { return true; }
};
} // namespace SamplerKit::HWTests
