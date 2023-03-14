#pragma once
#include "drivers/adc_builtin_conf.hh"
#include "drivers/pin.hh"
#include <array>

namespace Brain::Pin
{
using PinDef = mdrivlib::PinDef;
using GPIO = mdrivlib::GPIO;
using PinNum = mdrivlib::PinNum;
using PinAF = mdrivlib::PinAF;
using AdcChanNum = mdrivlib::AdcChanNum;

constexpr inline PinDef A0{GPIO::A, PinNum::_4};
constexpr inline PinDef A1{GPIO::A, PinNum::_5};
constexpr inline PinDef A2{GPIO::C, PinNum::_2};
constexpr inline PinDef A3{GPIO::C, PinNum::_3};
constexpr inline PinDef A4{GPIO::A, PinNum::_0};
constexpr inline PinDef A5{GPIO::A, PinNum::_1};
constexpr inline PinDef A6{GPIO::C, PinNum::_4};
constexpr inline PinDef A7{GPIO::C, PinNum::_0};
constexpr inline PinDef A8{GPIO::C, PinNum::_1};
constexpr inline PinDef A9{GPIO::A, PinNum::_6};

constexpr inline AdcChanNum A0AdcChan{AdcChanNum::_4};
constexpr inline AdcChanNum A1AdcChan{AdcChanNum::_5};
constexpr inline AdcChanNum A2AdcChan{AdcChanNum::_12};
constexpr inline AdcChanNum A3AdcChan{AdcChanNum::_13};
constexpr inline AdcChanNum A4AdcChan{AdcChanNum::_0};
constexpr inline AdcChanNum A5AdcChan{AdcChanNum::_1};
constexpr inline AdcChanNum A6AdcChan{AdcChanNum::_14};
constexpr inline AdcChanNum A7AdcChan{AdcChanNum::_10};
constexpr inline AdcChanNum A8AdcChan{AdcChanNum::_11};
constexpr inline AdcChanNum A9AdcChan{AdcChanNum::_6};

constexpr inline PinDef D0{GPIO::I, PinNum::_7};
constexpr inline PinDef D1{GPIO::I, PinNum::_6};
constexpr inline PinDef D2{GPIO::I, PinNum::_5};
constexpr inline PinDef D3{GPIO::B, PinNum::_8};
constexpr inline PinDef D4{GPIO::D, PinNum::_2};
constexpr inline PinDef D5{GPIO::I, PinNum::_0};
constexpr inline PinDef D6{GPIO::I, PinNum::_1};
constexpr inline PinDef D7{GPIO::I, PinNum::_2};
constexpr inline PinDef D8{GPIO::I, PinNum::_3};
constexpr inline PinDef D9{GPIO::C, PinNum::_12};
constexpr inline PinDef D10{GPIO::A, PinNum::_15};
constexpr inline PinDef D11{GPIO::C, PinNum::_7};
constexpr inline PinDef D12{GPIO::A, PinNum::_8};
constexpr inline PinDef D13{GPIO::H, PinNum::_4};
constexpr inline PinDef D14{GPIO::A, PinNum::_2};
constexpr inline PinDef D15{GPIO::H, PinNum::_5};
constexpr inline PinDef D16{GPIO::C, PinNum::_8};
constexpr inline PinDef D17{GPIO::C, PinNum::_9};
constexpr inline PinDef D18{GPIO::C, PinNum::_10};
constexpr inline PinDef D19{GPIO::C, PinNum::_11};

constexpr inline PinDef D16SdmmcDat0AF = {D16.gpio, D16.pin, PinAF::AltFunc12};
constexpr inline PinDef D17SdmmcDat1AF = {D17.gpio, D17.pin, PinAF::AltFunc12};
constexpr inline PinDef D18SdmmcDat2AF = {D18.gpio, D18.pin, PinAF::AltFunc12};
constexpr inline PinDef D19SdmmcDat3AF = {D19.gpio, D19.pin, PinAF::AltFunc12};
constexpr inline PinDef D9SdmmcClkAF = {D9.gpio, D9.pin, PinAF::AltFunc12};
constexpr inline PinDef D4SdmmcCmdAF = {D4.gpio, D4.pin, PinAF::AltFunc12};
constexpr inline uint32_t SdmmcPeriphNum = 1;
constexpr inline uint32_t SdmmcMaxSpeed = 24'000'000;

// TODO: Add AF for all other pins (DAC, I2C, SPI, UART, etc)

constexpr inline PinDef Debug2{GPIO::H, PinNum::_15};
constexpr inline PinDef Debug3{GPIO::D, PinNum::_3};
constexpr inline PinDef ConsoleUartTX{GPIO::H, PinNum::_13, PinAF::AltFunc8};
constexpr inline PinDef ConsoleUartRX{GPIO::H, PinNum::_14, PinAF::AltFunc8};

constexpr inline uint32_t ConsoleUartBaseAddr = UART4_BASE;

// TODO: do we put SAI, Codec I2C, Codec Reset here?

} // namespace Brain::Pin
