#include "cv_calibration.hh"
#include "doctest.h"

TEST_CASE("Normal operation") {
	using namespace SamplerKit;

	Flags flags;
	CVCalibrator cvcal{flags};

	cvcal.reset();
	flags.take(Flag::CVCalibrationFail);
	flags.take(Flag::EnterCVCalibrationStep2);

	float adc0V = 2060.f;
	float adc2V = 1250.f;
	float adc4V = 415.f;

	for (int i = 0; i < 100; i++)
		cvcal.update(adc0V);
	CHECK(!flags.take(Flag::CVCalibrationFail));

	flags.set(Flag::StepCVCalibration);

	for (int i = 0; i < 100; i++)
		cvcal.update(adc2V);

	CHECK(!flags.take(Flag::CVCalibrationFail));
	CHECK(flags.take(Flag::EnterCVCalibrationStep2));
}
