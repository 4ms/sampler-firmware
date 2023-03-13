#pragma once
#include "conf/board_conf.hh"
#include "libhwtests/HardwareTestUtil.hh"
#include "stm32xx.h"

namespace SamplerKit::HWTests
{
struct UtilIF {
	static bool main_button_pressed() {
		return Board::PlayButton::PinT::read();
		// return Board::RevButton::PinT::read();
	}
	static void delay_ms(uint32_t ms) { HAL_Delay(ms); }
	static void set_main_button_led(bool turn_on) { Board::PlayG::set(turn_on); }
};

using Util = HardwareTestUtil<UtilIF>;
} // namespace SamplerKit::HWTests
