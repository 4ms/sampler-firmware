#include "console.hh"
// #include "../lib/printf/printf.h"
// #include <cstdarg>

void Console::putchar(char c) { console.putchar(c); }

// void Console::log(const char *format, ...) {
// 	va_list va;
// 	va_start(va, format);
// 	vprintf_(format, va);
// 	va_end(va);
// }

extern "C" void _putchar(char c) { Console::putchar(c); }
