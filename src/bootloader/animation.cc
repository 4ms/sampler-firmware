#include "bootloader/animation.hh"
#include "bootloader/leds.hh"
#include "settings.h"
#include <cstdint>

extern volatile uint32_t systmr;

void animate(Animations animation_type) {
	uint32_t cur_tm = systmr;
	static uint32_t last_tm = 0;
	static uint8_t ctr = 0;
	uint32_t step_time = 500 * TICKS_PER_MS;

	switch (animation_type) {

		case ANI_RESET:
			set_rgb_led(RgbLeds::Ping, Palette::Black);
			set_rgb_led(RgbLeds::Cycle, Palette::Black);

			last_tm = cur_tm;
			ctr = 0;
			break;

		case ANI_SUCCESS:
			if (ctr == 0) {
				set_rgb_led(RgbLeds::Ping, Palette::Red);
				set_rgb_led(RgbLeds::Cycle, Palette::Red);
			} else if (ctr == 1) {
				set_rgb_led(RgbLeds::Ping, Palette::Yellow);
				set_rgb_led(RgbLeds::Cycle, Palette::Yellow);
			} else if (ctr == 2) {
				set_rgb_led(RgbLeds::Ping, Palette::White);
				set_rgb_led(RgbLeds::Cycle, Palette::White);
			} else if (ctr == 3) {
				set_rgb_led(RgbLeds::Ping, Palette::Cyan);
				set_rgb_led(RgbLeds::Cycle, Palette::Cyan);
			} else if (ctr == 4) {
				set_rgb_led(RgbLeds::Ping, Palette::Green);
				set_rgb_led(RgbLeds::Cycle, Palette::Green);
			} else if (ctr == 5) {
				set_rgb_led(RgbLeds::Ping, Palette::Blue);
				set_rgb_led(RgbLeds::Cycle, Palette::Blue);
			} else
				ctr = 0;
			break;

		case ANI_WAITING:
			//Flash button green/off when waiting
			if (ctr == 0)
				set_rgb_led(RgbLeds::Ping, Palette::Black);
			else if (ctr == 1)
				set_rgb_led(RgbLeds::Ping, Palette::Green);
			else
				ctr = 0;
			break;

		case ANI_RECEIVING:
			step_time = 200 * TICKS_PER_MS;
			if (ctr == 0) {
				set_rgb_led(RgbLeds::Ping, Palette::Blue);
				set_rgb_led(RgbLeds::Cycle, Palette::White);
			} else if (ctr == 1) {
				set_rgb_led(RgbLeds::Ping, Palette::White);
				set_rgb_led(RgbLeds::Cycle, Palette::Blue);
			} else
				ctr = 0;
			break;

		case ANI_SYNC:
			step_time = 100 * TICKS_PER_MS;
			if (ctr == 0) {
				set_rgb_led(RgbLeds::Ping, Palette::Black);
				set_rgb_led(RgbLeds::Cycle, Palette::Black);
			} else if (ctr == 1) {
				set_rgb_led(RgbLeds::Ping, Palette::Magenta);
				set_rgb_led(RgbLeds::Cycle, Palette::Black);
			} else
				ctr = 0;
			break;

		case ANI_WRITING:
			step_time = 100 * TICKS_PER_MS;
			if (ctr == 0) {
				set_rgb_led(RgbLeds::Ping, Palette::Black);
				set_rgb_led(RgbLeds::Cycle, Palette::Yellow);
			} else if (ctr == 1) {
				set_rgb_led(RgbLeds::Ping, Palette::Yellow);
				set_rgb_led(RgbLeds::Cycle, Palette::Black);
			} else
				ctr = 0;
			break;

		case ANI_FAIL_ERR:
			step_time = 100 * TICKS_PER_MS;
			if (ctr == 0) {
				set_rgb_led(RgbLeds::Ping, Palette::Black);
				set_rgb_led(RgbLeds::Cycle, Palette::Red);
			} else if (ctr == 1) {
				set_rgb_led(RgbLeds::Ping, Palette::Red);
				set_rgb_led(RgbLeds::Cycle, Palette::Black);
			} else
				ctr = 0;
			break;

		default:
			break;
	}

	if ((cur_tm - last_tm) > step_time) {
		ctr++;
		last_tm = cur_tm;
	}
}
