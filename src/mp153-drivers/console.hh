#pragma once
#include "drivers/uart.hh"

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

struct Console {
	static inline mdrivlib::Uart<uart_conf> console;

	static void putchar(char c);

	static void log(const char *format, ...);

	Console() { console.init(); }
	static void init() { console.init(); }
};
