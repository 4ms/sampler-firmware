#pragma once
#include "brain_conf.hh"
#include "brain_pinout.hh"
#include "drivers/adc_builtin_conf.hh"
#include "drivers/debounced_switch.hh"
#include "drivers/pin.hh"
#include "drivers/rgbled.hh"
#include "drivers/tim_pwm.hh"
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

using namespace Brain;

using PlayButton = mdrivlib::DebouncedSwitch<Pin::D12, Inverted>;
using BankButton = mdrivlib::DebouncedSwitch<Pin::D6, Inverted>;
using RevButton = mdrivlib::DebouncedSwitch<Pin::A6, Inverted>;

using PlayJack = mdrivlib::DebouncedPin<Pin::D13, Normal>;
using RevJack = mdrivlib::DebouncedPin<Pin::D2, Normal>;

using PlayR = mdrivlib::FPin<Pin::D8.gpio, Pin::D8.pin, Output, Inverted>;
using PlayG = mdrivlib::FPin<Pin::D11.gpio, Pin::D11.pin, Output, Inverted>;
using PlayB = mdrivlib::FPin<Pin::D14.gpio, Pin::D14.pin, Output, Inverted>;
using PlayLED = mdrivlib::MixedRgbLed<PlayR, PlayG, PlayB>;

using RevR = mdrivlib::FPin<Pin::D7.gpio, Pin::D7.pin, Output, Inverted>;
using RevG = mdrivlib::FPin<Pin::D10.gpio, Pin::D10.pin, Output, Inverted>;
using RevB = mdrivlib::FPin<Pin::D3.gpio, Pin::D3.pin, Output, Inverted>;
using RevLED = mdrivlib::MixedRgbLed<RevR, RevG, RevB>;

using BankR = mdrivlib::FPin<Pin::D1.gpio, Pin::D1.pin, Output, Inverted>;
using BankG = mdrivlib::FPin<Pin::D0.gpio, Pin::D0.pin, Output, Inverted>;
using BankB = mdrivlib::FPin<Pin::D5.gpio, Pin::D5.pin, Output, Inverted>;
using BankLED = mdrivlib::MixedRgbLed<BankR, BankG, BankB>;

// using PlayRPwm = mdrivlib::TimPwmChan<Pin::D8PwmConf>; // R = D8, has no PWM on F723-p3 or mp1-p3
using PlayGPwm = mdrivlib::TimPwmChan<Pin::D11PwmConf>;
using PlayBPwm = mdrivlib::TimPwmChan<Pin::D14PwmConf>;
using PlayPWM = mdrivlib::MixedRgbLed<PlayR, PlayGPwm, PlayBPwm>;

// using RevRPwm = mdrivlib::TimPwmChan<Pin::D7PwmConf>;// R = D7, has no PWM on mp1-p3
using RevGPwm = mdrivlib::TimPwmChan<Pin::D10PwmConf>;
using RevBPwm = mdrivlib::TimPwmChan<Pin::D3PwmConf>;
using RevPWM = mdrivlib::MixedRgbLed<RevR, RevGPwm, RevBPwm>;

using BankRPwm = mdrivlib::TimPwmChan<Pin::D1PwmConf>;
using BankGPwm = mdrivlib::TimPwmChan<Pin::D0PwmConf>;
// using BankBPwm = mdrivlib::TimPwmChan<Pin::D5PwmConf>;// Blue = D5, which has no PWM on mp1-p3
using BankPWM = mdrivlib::MixedRgbLed<BankRPwm, BankGPwm, BankB>;

using EndOut = mdrivlib::FPin<Pin::D15.gpio, Pin::D15.pin, Output, Normal>;

constexpr std::array<AdcChannelConf, NumPots> PotAdcChans = {{
	{Pin::A1, Pin::A1AdcChan, PitchPot, Brain::AdcSampTime},
	{Pin::A3, Pin::A3AdcChan, StartPot, Brain::AdcSampTime},
	{Pin::A7, Pin::A7AdcChan, LengthPot, Brain::AdcSampTime},
	{Pin::A9, Pin::A9AdcChan, SamplePot, Brain::AdcSampTime},
}};

constexpr std::array<AdcChannelConf, NumCVs> CVAdcChans = {{
	{Pin::A2, Pin::A2AdcChan, PitchCV, Brain::AdcSampTime},
	{Pin::A0, Pin::A0AdcChan, StartCV, Brain::AdcSampTime},
	{Pin::A5, Pin::A5AdcChan, LengthCV, Brain::AdcSampTime},
	{Pin::A8, Pin::A8AdcChan, SampleCV, Brain::AdcSampTime},
	{Pin::A4, Pin::A4AdcChan, BankCV, Brain::AdcSampTime},
}};

} // namespace SamplerKit::Board
