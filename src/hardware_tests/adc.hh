#pragma once
#include "conf/board_conf.hh"
#include "controls.hh"
#include "hardware_tests/util.hh"
#include "libhwtests/AdcChecker.hh"
#include "libhwtests/AdcRangeChecker.hh"

namespace SamplerKit::HWTests
{

struct TestADCs : IAdcChecker {
	static constexpr AdcRangeCheckerBounds bounds{
		.center_val = 2048,
		.center_width = 20,
		.center_check_counts = 100,
		.min_val = 12,
		.max_val = 4086,
	};

	Controls &controls;

	TestADCs(Controls &controls)
		: IAdcChecker{bounds, NumPots, 1, NumCVs - 1}
		, controls{controls} {}

	uint32_t get_adc_reading(uint8_t adc_i, AdcType adctype) override {
		if (adctype == AdcType::Pot)
			return controls.read_pot(static_cast<PotAdcElement>(adc_i));
		if (adctype == AdcType::BipolarCV)
			return controls.read_cv(PitchCV);
		if (adctype == AdcType::UnipolarCV)
			return controls.read_cv(static_cast<CVAdcElement>(adc_i + 1));
		return 0;
	}

	void set_indicator(uint8_t adc_i, AdcType adctype, AdcCheckState state) override {
		if (state == AdcCheckState::NoCoverage) {
			Board::BankLED{}.set_color(Colors::red);
			Board::PlayLED{}.set_color(Colors::green);
			Board::RevLED{}.set_color(Colors::red);
		}

		if (state == AdcCheckState::AtMin)
			Board::BankLED{}.set_color(Colors::off);

		if (state == AdcCheckState::AtMax)
			Board::RevLED{}.set_color(Colors::off);

		if (state == AdcCheckState::AtCenter)
			Board::PlayLED{}.set_color(Colors::off);

		if (state == AdcCheckState::Elsewhere)
			Board::PlayLED{}.set_color(Colors::green);

		if (state == AdcCheckState::FullyCovered) {
			Board::PlayLED{}.set_color(Colors::off);
			Board::RevLED{}.set_color(Colors::off);
			Board::BankLED{}.set_color(Colors::off);
		}
	}

	void pause_between_steps() override { HAL_Delay(10); }

	void show_multiple_nonzeros_error() override { Board::PlayLED{}.set_color(Colors::red); }

	bool button_to_skip_step() override {
		controls.play_button.update();
		return controls.play_button.is_just_pressed();
	}

	void delay_ms(uint32_t x) override { HAL_Delay(x); }
};
} // namespace SamplerKit::HWTests
