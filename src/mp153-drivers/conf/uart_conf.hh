#pragma once
#include "drivers/uart_conf.hh"

constexpr inline UartConf uart_conf{
	.base_addr = USART6_BASE,
	.TXPin = {mdrivlib::GPIO::C, mdrivlib::PinNum::_6, mdrivlib::PinAF::AltFunc7},
	.RXPin = {mdrivlib::GPIO::C, mdrivlib::PinNum::_7, mdrivlib::PinAF::AltFunc7},
	.mode = UartConf::Mode::TXRX,
	.baud = 115200,
	.wordlen = 8,
	.parity = UartConf::Parity::None,
	.stopbits = UartConf::StopBits::_1,

};
