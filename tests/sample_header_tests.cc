#include "doctest.h"
#include "src/sample_header.hh"
#include <cstdio>

TEST_CASE("Sample header") {
	//
	FILE *file = fopen("tests/test.wav", "r");
	CHECK(file != nullptr);

	Sample s;

	auto err = SamplerKit::load_sample_header(&s, file);
	CHECK(err == 0);
	CHECK(s.sampleRate == 48000);
	CHECK(s.sampleSize == 211812);
	CHECK(s.blockAlign == 4);
	CHECK(s.numChannels == 2);
	CHECK(s.num_cues == 4);
	CHECK(s.cue[0] == 1);
	CHECK(s.cue[1] == 1208);
	CHECK(s.cue[2] == 1495);
	CHECK(s.cue[3] == 4940);

	fclose(file);
}
