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

//
constexpr inline PinDef ConsoleUartTX{GPIO::C, PinNum::_6, PinAF::AltFunc7};
constexpr inline PinDef ConsoleUartRX{GPIO::C, PinNum::_7, PinAF::AltFunc7};
constexpr inline uint32_t ConsoleUartBaseAddr = USART6_BASE;
} // namespace Brain::Pin
