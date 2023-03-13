#pragma once
#include "analog_in_ext.hh"
#include "conf/board_conf.hh"
#include "conf/brain_conf.hh"
#include "debug.hh"
#include "drivers/adc_builtin.hh"
#include "drivers/debounced_switch.hh"
#include "elements.hh"
#include "util/filter.hh"

namespace SamplerKit
{

class Controls {
	static constexpr bool debug = false;

	template<typename ConfT>
	using AdcDmaPeriph = mdrivlib::AdcDmaPeriph<ConfT>;

	// ADCs (Pots and CV):
	std::array<uint16_t, NumCVs> cv_adc_buffer;
	AdcDmaPeriph<Brain::CVAdcConf> cv_adcs{cv_adc_buffer, Board::CVAdcChans};

	std::array<uint16_t, NumPots> pot_adc_buffer;
	AdcDmaPeriph<Brain::PotAdcConf> pot_adcs{pot_adc_buffer, Board::PotAdcChans};

	std::array<Oversampler<16, uint16_t>, NumPots> pots;
	std::array<Oversampler<8, uint16_t>, NumCVs> cvs;

public:
	Controls() = default;

	// Buttons/Switches:
	Board::PlayButton play_button;
	Board::RevButton rev_button;
	Board::BankButton bank_button;

	// Trig Jacks
	Board::PlayJack play_jack;
	Board::RevJack rev_jack;

	Board::EndOut end_out;

	// LEDs:
	Board::PlayLED play_led;
	Board::RevLED rev_led;
	Board::BankLED bank_led;

	enum class SwitchPos { Invalid = 0b00, Up = 0b01, Down = 0b10, Center = 0b11 };

	uint16_t read_pot(PotAdcElement adcnum) {
		if constexpr (debug)
			return pot_adc_buffer[adcnum];
		else
			return pots[adcnum].val();
	}
	uint16_t read_cv(CVAdcElement adcnum) {
		if constexpr (debug)
			return cv_adc_buffer[adcnum];
		else
			return cvs[adcnum].val();
	}

	// SwitchPos read_time_switch() { return static_cast<SwitchPos>(time_switch.read()); }

	void start() {
		if constexpr (!debug) {
			pot_adcs.register_callback([this] {
				for (unsigned i = 0; auto &pot : pots)
					pot.add_val(pot_adc_buffer[i++]);
			});
			cv_adcs.register_callback([this] {
				for (unsigned i = 0; auto &cv : cvs)
					cv.add_val(cv_adc_buffer[i++]);
			});
		}
		pot_adcs.start();
		cv_adcs.start();
	}

	void update() {
		play_button.update();
		rev_button.update();
		bank_button.update();
		play_jack.update();
		rev_jack.update();
	}

	GCC_OPTIMIZE_OFF
	void test() {
		play_led.set_color(Colors::off);
		play_led.set_color(Colors::red);
		play_led.set_color(Colors::blue);
		play_led.set_color(Colors::green);
		play_led.set_color(Colors::white);
		play_led.set_color(Colors::off);

		rev_led.set_color(Colors::off);
		rev_led.set_color(Colors::red);
		rev_led.set_color(Colors::blue);
		rev_led.set_color(Colors::green);
		rev_led.set_color(Colors::white);
		rev_led.set_color(Colors::off);

		bank_led.set_color(Colors::off);
		bank_led.set_color(Colors::red);
		bank_led.set_color(Colors::blue);
		bank_led.set_color(Colors::green);
		bank_led.set_color(Colors::white);
		bank_led.set_color(Colors::off);

		end_out.high();
		end_out.low();
		end_out.high();
		end_out.low();

		play_jack.is_high();
		rev_jack.is_high();

		play_button.update();
		play_button.is_pressed();
		play_button.update();
		play_button.is_pressed();

		rev_button.update();
		rev_button.is_pressed();
		rev_button.update();
		rev_button.is_pressed();

		bank_button.update();
		bank_button.is_pressed();
		bank_button.update();
		bank_button.is_pressed();
	}
};
} // namespace SamplerKit
