#pragma once
#include "drivers/adc_builtin_conf.hh"
#include "drivers/pin.hh"
#include <array>

namespace Brain::Pin
{
using PinDef = mdrivlib::PinDef;
using GPIO = mdrivlib::GPIO;
using PinNum = mdrivlib::PinNum;
using AdcChanNum = mdrivlib::AdcChanNum;

constexpr inline PinDef A0{GPIO::A, PinNum::_2};
constexpr inline PinDef A1{GPIO::A, PinNum::_6};
constexpr inline PinDef A2{GPIO::A, PinNum::_4};
constexpr inline PinDef A3{GPIO::A, PinNum::_5};
constexpr inline PinDef A4{GPIO::A, PinNum::_3};
constexpr inline PinDef A5{GPIO::A, PinNum::_7};

constexpr inline AdcChanNum A0_adc{AdcChanNum::_14};
constexpr inline AdcChanNum A1_adc{AdcChanNum::_3};
constexpr inline AdcChanNum A2_adc{AdcChanNum::_18};
constexpr inline AdcChanNum A3_adc{AdcChanNum::_14};
constexpr inline AdcChanNum A4_adc{AdcChanNum::_14};
constexpr inline AdcChanNum A5_adc{AdcChanNum::_14};

//
constexpr inline auto AdcSampTime = mdrivlib::AdcSamplingTime::_480Cycles;
constexpr std::array<mdrivlib::AdcChannelConf, 4> PotAdcChans = {{
	{{GPIO::A, PinNum::_5}, AdcChanNum::_5, 0, AdcSampTime},
	{{GPIO::C, PinNum::_3}, AdcChanNum::_13, 1, AdcSampTime},
	{{GPIO::C, PinNum::_0}, AdcChanNum::_10, 2, AdcSampTime},
	{{GPIO::A, PinNum::_6}, AdcChanNum::_6, 3, AdcSampTime},
}};
} // namespace Brain::Pin
