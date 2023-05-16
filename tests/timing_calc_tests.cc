#include "doctest.h"

#include "timing_calcs.hh"

TEST_CASE("fade rate") {

	CHECK(SamplerKit::TimingCalcs::calc_fade_updown_rate(48000.f, 16.f, 0) == 1. / 16.);
	CHECK(SamplerKit::TimingCalcs::calc_fade_updown_rate(48000.f, 16.f, 1) < 1. / 16.);
	CHECK(SamplerKit::TimingCalcs::calc_fade_updown_rate(48000.f, 16.f, 1000) == doctest::Approx(1. / 48000.));
}
