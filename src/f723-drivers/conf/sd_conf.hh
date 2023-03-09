#pragma once
#include "drivers/sdcard_conf.hh"

namespace SamplerKit::Board
{
using PinDef = mdrivlib::PinDef;
using GPIO = mdrivlib::GPIO;
using PinNum = mdrivlib::PinNum;
using PinAF = mdrivlib::PinAF;

struct SDCardConf : mdrivlib::DefaultSDCardConf {
	static constexpr uint32_t SDPeriphNum = 1;
	static constexpr uint32_t speed_hz = 25'000'000;
	static constexpr Width width = Wide4;
	static constexpr PinDef D0{GPIO::C, PinNum::_8, PinAF::AltFunc12};
	static constexpr PinDef D1{GPIO::C, PinNum::_9, PinAF::AltFunc12};
	static constexpr PinDef D2{GPIO::C, PinNum::_10, PinAF::AltFunc12};
	static constexpr PinDef D3{GPIO::C, PinNum::_11, PinAF::AltFunc12};
	static constexpr PinDef SCLK{GPIO::C, PinNum::_12, PinAF::AltFunc12};
	static constexpr PinDef CMD{GPIO::D, PinNum::_2, PinAF::AltFunc12};
	//
};
} // namespace SamplerKit::Board
