#pragma once
#include "conf/uart_conf.hh"
#include "drivers/uart.hh"

struct Console {
	static inline mdrivlib::Uart<uart_conf> console;

	static void putchar(char c);

	static void log(const char *format, ...);

	Console() { console.init(); }
	static void init() { console.init(); }
};
