#include "doctest.h"
#include "src/sample_header.hh"
#include <cstdio>

TEST_CASE("Sample header") {
	//
	FILE *f = fopen("tests/test.wav", "r");
	CHECK(f != nullptr);

	Sample s;

	SUBCASE("valid header") {
		//
	}

	fclose(f);
}
