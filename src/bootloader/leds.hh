#pragma once

enum class RgbLeds {
	Ping,
	Cycle,
	EnvA,
	EnvB,
};

enum class Palette {
	Black = 0b000,
	Blue = 0b001,
	Green = 0b010,
	Cyan = 0b011,
	Red = 0b100,
	Magenta = 0b101,
	Yellow = 0b110,
	White = 0b111,
};

void init_leds();
void set_rgb_led(RgbLeds rgb_led_id, Palette color_id);

