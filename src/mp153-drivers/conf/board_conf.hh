#pragma once
#include "drivers/adc_builtin_conf.hh"
#include "drivers/debounced_switch.hh"
#include "drivers/pin.hh"
#include "drivers/rgbled.hh"
#include "elements.hh"
#include <array>

namespace SamplerKit::Board
{

using mdrivlib::AdcChannelConf;
using mdrivlib::AdcChanNum;
using mdrivlib::GPIO;
using mdrivlib::PinDef;
using mdrivlib::PinNum;
using enum mdrivlib::PinPolarity;
using enum mdrivlib::PinMode;

using PlayButton = mdrivlib::DebouncedSwitch<PinDef{GPIO::E, PinNum::_4}, Inverted>;
using BankButton = mdrivlib::DebouncedSwitch<PinDef{GPIO::B, PinNum::_10}, Inverted>;
using RevButton = mdrivlib::DebouncedSwitch<PinDef{GPIO::B, PinNum::_0}, Inverted>;

using PlayJack = mdrivlib::DebouncedPin<PinDef{GPIO::D, PinNum::_1}, Normal>;
using RevJack = mdrivlib::DebouncedPin<PinDef{GPIO::B, PinNum::_7}, Normal>;

using PlayR = mdrivlib::FPin<GPIO::C, PinNum::_1, Output, Inverted>;
using PlayG = mdrivlib::FPin<GPIO::B, PinNum::_1, Output, Inverted>;
using PlayB = mdrivlib::FPin<GPIO::A, PinNum::_1, Output, Inverted>;
using PlayLED = mdrivlib::MixedRgbLed<PlayR, PlayG, PlayB>;

using RevR = mdrivlib::FPin<GPIO::C, PinNum::_2, Output, Inverted>;
using RevG = mdrivlib::FPin<GPIO::A, PinNum::_8, Output, Inverted>;
using RevB = mdrivlib::FPin<GPIO::B, PinNum::_4, Output, Inverted>;
using RevLED = mdrivlib::MixedRgbLed<RevR, RevG, RevB>;

using BankR = mdrivlib::FPin<GPIO::E, PinNum::_13, Output, Inverted>;
using BankG = mdrivlib::FPin<GPIO::A, PinNum::_9, Output, Inverted>;
using BankB = mdrivlib::FPin<GPIO::B, PinNum::_12, Output, Inverted>;
using BankLED = mdrivlib::MixedRgbLed<BankR, BankG, BankB>;

using EndOut = mdrivlib::FPin<GPIO::D, PinNum::_0, Output, Normal>;

constexpr inline auto AdcSampTime = mdrivlib::AdcSamplingTime::_2Cycles;

constexpr std::array<AdcChannelConf, NumPots> PotAdcChans = {{
	{{GPIO::A, PinNum::_6}, AdcChanNum::_3, PitchPot, AdcSampTime},
	{{GPIO::A, PinNum::_5}, AdcChanNum::_19, StartPot, AdcSampTime},
	{{GPIO::C, PinNum::_5}, AdcChanNum::_8, LengthPot, AdcSampTime},
	{{GPIO::C, PinNum::_0}, AdcChanNum::_10, SamplePot, AdcSampTime},
}};

constexpr std::array<AdcChannelConf, NumCVs> CVAdcChans = {{
	{{GPIO::A, PinNum::_4}, AdcChanNum::_18, PitchCV, AdcSampTime},
	{{GPIO::A, PinNum::_2}, AdcChanNum::_14, StartCV, AdcSampTime},
	{{GPIO::A, PinNum::_7}, AdcChanNum::_7, LengthCV, AdcSampTime},
	{{GPIO::C, PinNum::_3}, AdcChanNum::_13, SampleCV, AdcSampTime},
	{{GPIO::A, PinNum::_3}, AdcChanNum::_15, BankCV, AdcSampTime},
}};

} // namespace SamplerKit::Board
