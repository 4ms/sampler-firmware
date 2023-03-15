#pragma once
#include "brain_conf.hh"
#include "conf/board_conf.hh"
#include "debug.hh"
#include "drivers/adc_builtin.hh"
#include "drivers/analog_in_ext.hh"
#include "drivers/debounced_switch.hh"
#include "elements.hh"
#include "util/filter.hh"

namespace SamplerKit
{

class Controls {

	template<typename ConfT>
	using AdcDmaPeriph = mdrivlib::AdcDmaPeriph<ConfT>;

	// ADCs (Pots and CV):
	static inline __attribute__((section(".noncachable"))) std::array<uint16_t, NumCVs> cv_adc_buffer;
	AdcDmaPeriph<Brain::CVAdcConf> cv_adcs{cv_adc_buffer, Board::CVAdcChans};

	static inline __attribute__((section(".noncachable"))) std::array<uint16_t, NumPots> pot_adc_buffer;
	AdcDmaPeriph<Brain::PotAdcConf> pot_adcs{pot_adc_buffer, Board::PotAdcChans};

	static constexpr bool hardware_oversampling = Brain::PotAdcConf::oversample;
	std::array<Oversampler<128, uint16_t>, NumPots> pots;
	std::array<Oversampler<32, uint16_t>, NumCVs> cvs;

public:
	Controls() {} //= default;

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
		if constexpr (hardware_oversampling)
			return pot_adc_buffer[adcnum];
		else
			return pots[adcnum].val();
	}
	uint16_t read_cv(CVAdcElement adcnum) {
		if constexpr (hardware_oversampling)
			return cv_adc_buffer[adcnum];
		else
			return cvs[adcnum].val();
	}

	void start() {
		if constexpr (!hardware_oversampling) {
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
};
} // namespace SamplerKit
