#include "doctest.h"
//
#define printf_ printf
#include <cstdio>
//
#include "cv_calibration.hh"

TEST_CASE("Normal operation") {
	using namespace SamplerKit;

	Flags flags;
	CVCalibrator cvcal{flags};

	cvcal.reset();
	flags.clear(Flag::CVCalibrationFail);
	flags.clear(Flag::CVCalibrationSuccess);
	flags.clear(Flag::EnterCVCalibrateMode);
	flags.clear(Flag::StepCVCalibration);
	flags.clear(Flag::CVCalibrationStep1Animate);
	flags.clear(Flag::CVCalibrationStep2Animate);
	flags.clear(Flag::CVCalibrationSuccessAnimate);
	flags.clear(Flag::CVCalibrationFailAnimate);

	float adc0V = 2060.f;
	float adc1V = 1665.f;
	float adc2V = 1250.f;
	float adc4V = 415.f;

	// Simulate accidentally still patched jack ==> fails
	SUBCASE("Unpatched Bad") {
		for (int i = 0; i < 100; i++)
			cvcal.update(adc1V);
		CHECK(flags.take(Flag::CVCalibrationFail));
	}

	// Simulate unpatched jack
	SUBCASE("Unpatched OK") {
		for (int i = 0; i < 100; i++)
			cvcal.update(adc0V);
		CHECK(!flags.take(Flag::CVCalibrationFail));

		// Simulate patching a C2 into jack
		for (int i = 0; i < 100; i++)
			cvcal.update(adc2V);
		CHECK(!flags.take(Flag::CVCalibrationFail));

		// Simulate pressing button to confirm C2 is on jack
		flags.set(Flag::StepCVCalibration);

		// C2 is still plugged in (for at least 32 counts)
		for (int i = 0; i < 100; i++)
			cvcal.update(adc2V);
		CHECK(!flags.take(Flag::CVCalibrationFail));

		// Simulate changing patched-in voltage to C4
		for (int i = 0; i < 100; i++)
			cvcal.update(adc4V);
		CHECK(!flags.take(Flag::CVCalibrationFail));

		// Simulate pressing button to confirm C4 is on jack
		flags.set(Flag::StepCVCalibration);

		// C4 is still plugged in (for at least 32 counts)
		for (int i = 0; i < 100; i++)
			cvcal.update(adc4V);
		CHECK(!flags.take(Flag::CVCalibrationFail));

		CHECK(flags.take(Flag::CVCalibrationSuccess));
	}
}
