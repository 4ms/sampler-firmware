#pragma once
#include "drivers/uart_conf.hh"

constexpr inline UartConf uart_conf{
	.base_addr = UART4_BASE,
	.TXPin = {mdrivlib::GPIO::H, mdrivlib::PinNum::_13, mdrivlib::PinAF::AltFunc8},
	.RXPin = {mdrivlib::GPIO::H, mdrivlib::PinNum::_14, mdrivlib::PinAF::AltFunc8},
	.mode = UartConf::Mode::TXRX,
	.baud = 115200,
	.wordlen = 8,
	.parity = UartConf::Parity::None,
	.stopbits = UartConf::StopBits::_1,
};
