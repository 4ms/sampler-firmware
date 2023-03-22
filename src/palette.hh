#pragma once
#include "util/colors.hh"

// enum LEDPalette {
// 	OFF,

// 	WHITE,	  // 1
// 	RED,	  // 2
// 	ORANGE,	  // 3
// 	YELLOW,	  // 4
// 	GREEN,	  // 5
// 	CYAN,	  // 6
// 	BLUE,	  // 7
// 	MAGENTA,  // 8
// 	LAVENDER, // 9
// 	PINK,	  // 10

// 	AQUA,
// 	GREENER,
// 	CYANER,
// 	DIM_WHITE,
// 	DIM_RED,
// 	MED_RED,

// 	NUM_LED_PALETTE
// };

// constexpr uint32_t LED_PALETTE[][3] = {
// 	{0, 0, 0}, // OFF

// 	{930, 950, 900}, // WHITE
// 	{1000, 0, 0},	 // RED
// 	{1000, 200, 0},	 // ORANGE
// 	{700, 650, 0},	 // YELLOW
// 	{0, 600, 60},	 // GREEN
// 	{0, 560, 1000},	 // CYAN
// 	{0, 0, 1000},	 // BLUE
// 	{1000, 0, 1000}, // MAGENTA
// 	{410, 0, 1000},	 // LAVENDER
// 	{150, 150, 200}, // PINK

// 	{0, 280, 150},	  // AQUA
// 	{550, 700, 0},	  // GREENER
// 	{200, 800, 1000}, // CYANER
// 	{100, 100, 100},  // DIM_WHITE
// 	{50, 0, 0},		  // DIM_RED
// 	{150, 0, 0}		  // MED_RED

// };

struct SamplerColors {
	static constexpr Color off = Color(0, 0, 0);
	static constexpr Color black = Color(0, 0, 0);
	static constexpr Color grey = Color(127, 127, 127);
	static constexpr Color white = Color(255, 255, 255);

	static constexpr Color red = Color(255, 0, 0);
	static constexpr Color pink = Color(0xFF, 0x69, 0xB4);
	static constexpr Color pale_pink = Color(255, 200, 200);
	static constexpr Color tangerine = Color(255, 200, 0);
	static constexpr Color orange = Color(255, 127, 0);
	static constexpr Color yellow = Color(255, 255, 0);
	static constexpr Color green = Color(0, 180, 0);
	static constexpr Color cyan = Color(0, 160, 200);
	static constexpr Color blue = Color(0, 0, 255);
	static constexpr Color purple = Color(255, 0, 255);
	static constexpr Color magenta = Color(200, 0, 100);
};

constexpr inline Color BankColors[] = {
	Colors::white,
	Colors::red,
	Colors::orange,
	Colors::yellow,
	Colors::green,
	Colors::cyan,
	Colors::blue,
	Colors::magenta,
	Colors::purple,
	Colors::grey,
};
