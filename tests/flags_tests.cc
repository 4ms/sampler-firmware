#include "doctest.h"

#include "flags.hh"
using namespace SamplerKit;

TEST_CASE("") {

	Flags f;

	auto check = [&f](Flag flag) {
		// init to 0
		CHECK_FALSE(f.read(flag));

		// set
		f.set(flag);
		CHECK(f.read(flag));
		CHECK(f.read(flag));

		// Only one bit was set
		for (uint32_t i = 0; i < Flags::NumFlags; i++) {
			if (static_cast<uint32_t>(flag) == i)
				continue;
			CHECK_FALSE(f.read(static_cast<Flag>(i)));
		}

		// clear
		f.clear(flag);
		CHECK_FALSE(f.read(flag));

		// Take
		CHECK_FALSE(f.take(flag));
		f.set(flag);
		CHECK(f.take(flag));
		CHECK_FALSE(f.take(flag));
	};

	check(Flag::LatchVoltOctCV);		  // 0
	check(Flag::ToggleStereoModeAnimate); // 31
	check(Flag::EnterLEDCalibrateMode);	  // 32
	check(Flag::StartFadeUp);			  // 32
	check(Flag::StartupParsing);
	check(Flag::WriteSettingsToSD);
}
