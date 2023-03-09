#include "drivers/uart.hh"

struct Console {
	static mdrivlib::Uart<UART4_BASE> console;

	static void putchar(char c);

	static void log(const char *format, ...);

	void console_init() {
		mdrivlib::Pin uart_tx{
			mdrivlib::GPIO::H, mdrivlib::PinNum::_13, mdrivlib::PinMode::Alt, mdrivlib::PinAF::AltFunc8};
		mdrivlib::Pin uart_rx{
			mdrivlib::GPIO::H, mdrivlib::PinNum::_14, mdrivlib::PinMode::Alt, mdrivlib::PinAF::AltFunc8};
	}
};
