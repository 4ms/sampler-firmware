#include "conf/board_conf.hh"
#include "hardware_tests/adc.hh"
#include "hardware_tests/buttons.hh"
#include "hardware_tests/leds.hh"
#include "hardware_tests/util.hh"
#include "stm32xx.h"

namespace SamplerKit::HWTests
{

void all_lights_off() {
	Board::PlayLED{}.set_color(Colors::off);
	Board::RevLED{}.set_color(Colors::off);
	Board::BankLED{}.set_color(Colors::off);
}

void run(Controls &controls) {
	all_lights_off();
	Util::pause_until_button_released();
	Util::flash_mainbut_until_pressed();

	TestLEDs ledtester;
	while (!ledtester.is_done())
		ledtester.run_test();

	TestButtons buttontester;
	while (buttontester.check())
		;

	TestADCs adctester{controls};
	adctester.run_test();
}
} // namespace SamplerKit::HWTests
