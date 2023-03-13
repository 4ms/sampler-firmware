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

using PlayButton = mdrivlib::DebouncedSwitch<PinDef{GPIO::I, PinNum::_3}, Inverted>;
using BankButton = mdrivlib::DebouncedSwitch<PinDef{GPIO::I, PinNum::_1}, Inverted>;
using RevButton = mdrivlib::DebouncedSwitch<PinDef{GPIO::C, PinNum::_4}, Inverted>;

using PlayJack = mdrivlib::DebouncedPin<PinDef{GPIO::H, PinNum::_4}, Normal>;
using RevJack = mdrivlib::DebouncedPin<PinDef{GPIO::I, PinNum::_5}, Normal>;

using PlayR = mdrivlib::FPin<GPIO::A, PinNum::_8, Output, Inverted>;
using PlayG = mdrivlib::FPin<GPIO::C, PinNum::_7, Output, Inverted>;
using PlayB = mdrivlib::FPin<GPIO::A, PinNum::_2, Output, Inverted>;
using PlayLED = mdrivlib::MixedRgbLed<PlayR, PlayG, PlayB>;

using RevR = mdrivlib::FPin<GPIO::I, PinNum::_2, Output, Inverted>;
using RevG = mdrivlib::FPin<GPIO::A, PinNum::_15, Output, Inverted>;
using RevB = mdrivlib::FPin<GPIO::B, PinNum::_8, Output, Inverted>;
using RevLED = mdrivlib::MixedRgbLed<RevR, RevG, RevB>;

using BankR = mdrivlib::FPin<GPIO::I, PinNum::_6, Output, Inverted>;
using BankG = mdrivlib::FPin<GPIO::I, PinNum::_7, Output, Inverted>;
using BankB = mdrivlib::FPin<GPIO::I, PinNum::_0, Output, Inverted>;
using BankLED = mdrivlib::MixedRgbLed<BankR, BankG, BankB>;

using EndOut = mdrivlib::FPin<GPIO::H, PinNum::_5, Output, Normal>;

constexpr inline auto AdcSampTime = mdrivlib::AdcSamplingTime::_480Cycles;

// constexpr auto PotConfs = std::to_array({
// 	AdcChannelConf{{GPIO::B, PinNum::_1}, mdrivlib::AdcChanNum::_5, PotA, AdcSampTime},
// 	AdcChannelConf{{GPIO::A, PinNum::_2}, mdrivlib::AdcChanNum::_14, PotB, AdcSampTime},
// 	AdcChannelConf{{GPIO::B, PinNum::_0}, mdrivlib::AdcChanNum::_9, PotC, AdcSampTime},
// 	AdcChannelConf{{GPIO::C, PinNum::_1}, mdrivlib::AdcChanNum::_11, PotD, AdcSampTime},
// 	AdcChannelConf{{GPIO::C, PinNum::_2}, mdrivlib::AdcChanNum::_12, PotE, AdcSampTime},
// 	AdcChannelConf{{GPIO::A, PinNum::_5}, mdrivlib::AdcChanNum::_19, PotF, AdcSampTime},
// 	AdcChannelConf{{GPIO::A, PinNum::_6}, mdrivlib::AdcChanNum::_3, PotU, AdcSampTime},
// 	AdcChannelConf{{GPIO::A, PinNum::_7}, mdrivlib::AdcChanNum::_7, PotV, AdcSampTime},
// 	AdcChannelConf{{GPIO::C, PinNum::_5}, mdrivlib::AdcChanNum::_8, PotW, AdcSampTime},
// 	AdcChannelConf{{GPIO::A, PinNum::_3}, mdrivlib::AdcChanNum::_15, PotX, AdcSampTime},
// 	AdcChannelConf{{GPIO::C, PinNum::_3}, mdrivlib::AdcChanNum::_13, PotY, AdcSampTime},
// 	AdcChannelConf{{GPIO::A, PinNum::_4}, mdrivlib::AdcChanNum::_18, PotZ, AdcSampTime},
// });
constexpr std::array<AdcChannelConf, NumPots> PotAdcChans = {{
	{{GPIO::A, PinNum::_5}, AdcChanNum::_5, PitchPot, AdcSampTime},
	{{GPIO::C, PinNum::_3}, AdcChanNum::_13, StartPot, AdcSampTime},
	{{GPIO::C, PinNum::_0}, AdcChanNum::_10, LengthPot, AdcSampTime},
	{{GPIO::A, PinNum::_6}, AdcChanNum::_6, SamplePot, AdcSampTime},
}};

constexpr std::array<AdcChannelConf, NumCVs> CVAdcChans = {{
	{{GPIO::C, PinNum::_2}, AdcChanNum::_12, PitchCV, AdcSampTime},
	{{GPIO::A, PinNum::_4}, AdcChanNum::_4, StartCV, AdcSampTime},
	{{GPIO::A, PinNum::_1}, AdcChanNum::_1, LengthCV, AdcSampTime},
	{{GPIO::C, PinNum::_1}, AdcChanNum::_11, SampleCV, AdcSampTime},
	{{GPIO::A, PinNum::_0}, AdcChanNum::_0, BankCV, AdcSampTime},
}};

} // namespace SamplerKit::Board
