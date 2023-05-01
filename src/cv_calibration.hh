#pragma once
#include "controls.hh"
#include "util/voct_calibrator.hh"

namespace SamplerKit
{

struct CVCalibrator {
	Controls &controls;
	VoctCalibrator<float> cal;

	CVCalibrator(Controls &controls)
		: controls{controls} {}

	void reset() { cal.reset(); }

	bool update() {
		//
		// if something
		return true;
	}
};

} // namespace SamplerKit
