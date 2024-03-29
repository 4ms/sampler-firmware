#include "doctest.h"
//
#define printf_ printf
#include <cstdio>
//
#include "sampler_calcs.hh"

TEST_CASE("align addr") {
	using namespace SamplerKit;
	CHECK(align_addr(0x123, 1) == 0x123);
	CHECK(align_addr(0x123, 2) == 0x120); // Why? memory alignment?
	CHECK(align_addr(0x123, 4) == 0x120);
}

TEST_CASE("calc_start_cuenum") {
	using namespace SamplerKit;
	Sample s;
	s.inst_start = 0;
	s.inst_end = 300;
	s.blockAlign = 2;
	s.num_cues = 4;
	s.cue[0] = 10;
	s.cue[1] = 30;
	s.cue[2] = 35;
	s.cue[3] = 100;

	CHECK(cue_pos(0, &s) == 0 * s.blockAlign);
	CHECK(cue_pos(1, &s) == 10 * s.blockAlign);
	CHECK(cue_pos(2, &s) == 30 * s.blockAlign);
	CHECK(cue_pos(3, &s) == 35 * s.blockAlign);
	CHECK(cue_pos(4, &s) == 100 * s.blockAlign);

	CHECK(calc_start_cuenum(0.0f, &s) == 0);
	CHECK(calc_start_cuenum(0.19f, &s) == 0);
	CHECK(calc_start_cuenum(0.251f, &s) == 1);
	CHECK(calc_start_cuenum(0.51f, &s) == 2);
	CHECK(calc_start_cuenum(0.751f, &s) == 3);
	CHECK(calc_start_cuenum(0.99f, &s) == 4);
	CHECK(calc_start_cuenum(1.0f, &s) == 4);

	SUBCASE("invalid cue returns -1") {
		// 0.51f  is cue #2, which is cue[1]:
		// Set cue[1] to be past the end of the sample data
		s.cue[1] = s.inst_end / s.blockAlign + 1;
		CHECK(calc_start_cuenum(0.5f, &s) == -1);
	}
}

TEST_CASE("calc_stop_cuenum") {
	using namespace SamplerKit;
	Sample s;
	s.inst_start = 0;
	s.inst_end = 500'000;
	s.blockAlign = 2;
	s.num_cues = 4;
	s.cue[0] = 10'000;
	s.cue[1] = 30'000;
	s.cue[2] = 80'000;
	s.cue[3] = 180'000;

	CHECK(calc_stop_cuenum(0, 0.0, &s) == 1);
	CHECK(calc_stop_cuenum(0, 0.1, &s) == 1);
	CHECK(calc_stop_cuenum(1, 0.1, &s) == 2);
	CHECK(calc_stop_cuenum(2, 0.1, &s) == 3);

	CHECK(calc_stop_cuenum(0, 0.26, &s) == 2);
	CHECK(calc_stop_cuenum(0, 0.52, &s) == 3);
	CHECK(calc_stop_cuenum(0, 0.76, &s) == 4);

	SUBCASE("stop cue past end returns -1") {
		CHECK(calc_stop_cuenum(1, 0.99, &s) == -1);
		CHECK(calc_stop_cuenum(2, 0.52, &s) == -1);
	}

	float rs = 1.0;
	float sr = 48000.0;
	uint32_t start_pos = 0;

	auto start_cue = 1; // s.cue[0]
	CHECK(calc_stop_point(0.51, rs, &s, start_pos, start_cue, sr) == s.cue[1] * s.blockAlign);
	CHECK(calc_stop_point(0.63, rs, &s, start_pos, start_cue, sr) == s.cue[2] * s.blockAlign);
	CHECK(calc_stop_point(0.76, rs, &s, start_pos, start_cue, sr) == s.cue[3] * s.blockAlign);
	CHECK(calc_stop_point(0.99, rs, &s, start_pos, start_cue, sr) == s.inst_end);

	SUBCASE("stop point is a minimum of two READ_BLOCKS from start point") {
		s.cue[1] = 15'000;
		auto expected = s.cue[0] * s.blockAlign + READ_BLOCK_SIZE * 2;
		CHECK(calc_stop_point(0.51, rs, &s, start_pos, start_cue, sr) == expected);
	}
}
