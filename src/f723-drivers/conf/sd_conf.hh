#pragma once
#include "drivers/sdcard_conf.hh"

namespace SamplerKit::Board
{
using namespace mdrivlib;

struct SDCardConf : DefaultSDCardConf {
	static constexpr uint32_t SDPeriphNum = 1;
	static constexpr uint32_t speed_hz = 25'000'000;
	static constexpr Width width = Wide4;
	static constexpr PinDef D0{};
	static constexpr PinDef D1{};
	static constexpr PinDef D2{};
	static constexpr PinDef D3{};
	static constexpr PinDef SCLK{};
	static constexpr PinDef CMD{};
	//
};
} // namespace SamplerKit::Board
