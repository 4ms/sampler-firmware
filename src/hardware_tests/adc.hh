#pragma once
#include "conf/board_conf.hh"
#include "console.hh"
#include "controls.hh"
#include "hardware_tests/util.hh"
#include "libhwtests/AdcChecker.hh"
#include "libhwtests/AdcRangeChecker.hh"
#include "printf.h"
#include <string_view>

namespace SamplerKit::HWTests
{

struct TestADCs : IAdcChecker {
	static constexpr AdcRangeCheckerBounds bounds{
		.center_val = 2048,
		.center_width = 20,
		.center_check_counts = 100,
		.min_val = Brain::PotAdcConf::min_value,
		.max_val = 4086,
	};

	uint32_t last_update = 0;

	Controls &controls;

	static constexpr std::string_view pot_names[] = {"Pitch", "StartPos", "Length", "Sample"};
	static constexpr std::string_view bi_cv_names[] = {"Pitch"};
	static constexpr std::string_view uni_cv_names[] = {"StartPos", "Length", "Sample", "Bank"};

	TestADCs(Controls &controls)
		: IAdcChecker{bounds, NumPots, 1, NumCVs - 1}
		, controls{controls} {}

	uint32_t get_adc_reading(uint8_t adc_i, AdcType adctype) override {
		uint16_t val = 0;

		if (adctype == AdcType::Pot)
			val = controls.read_pot(static_cast<PotAdcElement>(adc_i));

		if (adctype == AdcType::BipolarCV)
			val = controls.read_cv(PitchCV);

		if (adctype == AdcType::UnipolarCV)
			val = controls.read_cv(static_cast<CVAdcElement>(adc_i + 1));

		if ((HAL_GetTick() - last_update) > 100) {
			printf_("\x1b[2K\x1b[0G%d", val);
			last_update = HAL_GetTick();
		}
		return val;
	}

	void set_indicator(uint8_t adc_i, AdcType adctype, AdcCheckState state) override {
		if (state == AdcCheckState::NoCoverage) {
			if (adctype == AdcType::Pot)
				printf_("\nChecking Pot %.10s (#%d):\n", pot_names[adc_i].data(), adc_i);
			if (adctype == AdcType::BipolarCV)
				printf_("\nChecking Bipolar CV Jack %.10s (#%d):\n", bi_cv_names[adc_i].data(), adc_i);
			if (adctype == AdcType::UnipolarCV)
				printf_("\nChecking Unipolar CV Jack %.10s (#%d):\n", uni_cv_names[adc_i].data(), adc_i);

			Board::BankLED{}.set_color(Colors::red);
			Board::PlayLED{}.set_color(Colors::green);
			Board::RevLED{}.set_color(Colors::red);
		}

		if (state == AdcCheckState::AtMin) {
			Board::BankLED{}.set_color(Colors::off);
		}

		if (state == AdcCheckState::AtMax)
			Board::RevLED{}.set_color(Colors::off);

		if (state == AdcCheckState::AtCenter)
			Board::PlayLED{}.set_color(Colors::off);

		if (state == AdcCheckState::Elsewhere)
			Board::PlayLED{}.set_color(Colors::green);

		if (state == AdcCheckState::FullyCovered) {
			printf_("Done\n");
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
