#pragma once
// #include "drivers/rcc.hh"
#include "drivers/uart.hh"

constexpr inline UartConf uart_conf{
	.base_addr = UART4_BASE,
};

struct Console {
	static mdrivlib::Uart<uart_conf> console;

	static void putchar(char c);

	static void log(const char *format, ...);

	static void console_init() {
		console.init();
		mdrivlib::Pin uart_tx{
			mdrivlib::GPIO::H, mdrivlib::PinNum::_13, mdrivlib::PinMode::Alt, mdrivlib::PinAF::AltFunc8};
		mdrivlib::Pin uart_rx{
			mdrivlib::GPIO::H, mdrivlib::PinNum::_14, mdrivlib::PinMode::Alt, mdrivlib::PinAF::AltFunc8};
	}
};
