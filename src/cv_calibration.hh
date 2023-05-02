#pragma once
#include "flags.hh"
#include "log.hh"
#include "util/voct_calibrator.hh"
#include <cmath>

namespace SamplerKit
{

class CVCalibrator {
	Flags &flags;

	VoctCalibrator<float> cal;
	enum CVCalStep { C0, WaitC2, MeasureC2, WaitC4, MeasureC4, Done } cv_cal_step = C0;

	uint32_t accum_ctr = 0;
	float accum = 0;
	float avg_cv = 0.f;

public:
	CVCalibrator(Flags &flags)
		: flags{flags} {
		cal.reset();
	}

	void reset() {
		cv_cal_step = C0;
		cal.reset();
		accum_ctr = 0;
		accum = 0;
		avg_cv = 0.f;
	}

	void update(int16_t pitch_cv) {
		if (cv_cal_step == C0) {
			if (measure(pitch_cv, 2048.f)) {
				pr_log("C0: %f\n", (double)avg_cv);
				cal.record_measurement(VoctCalibrator<>::C0, avg_cv);
				cv_cal_step = WaitC2;
				reset_accum();
			}

		} else if (cv_cal_step == WaitC2) {
			if (flags.take(Flag::StepCVCalibration)) {
				pr_log("Start Measure C2\n");
				cv_cal_step = MeasureC2;
				reset_accum();
			}

		} else if (cv_cal_step == MeasureC2) {
			if (measure(pitch_cv, 1228.8f)) {
				pr_log("C2: %f\n", (double)avg_cv);
				cal.record_measurement(VoctCalibrator<>::C2, avg_cv);
				cv_cal_step = WaitC4;
				flags.set(Flag::CVCalibrationStep2Animate);
				reset_accum();
			}

		} else if (cv_cal_step == WaitC4) {
			if (flags.take(Flag::StepCVCalibration)) {
				pr_log("Start Measure C4\n");
				cv_cal_step = MeasureC4;
			}

		} else if (cv_cal_step == MeasureC4) {
			if (measure(pitch_cv, 409.6f)) {
				pr_log("C4: %f\n", (double)avg_cv);
				cal.record_measurement(VoctCalibrator<>::C4, avg_cv);
				cv_cal_step = Done;
				flags.set(Flag::CVCalibrationSuccess);
				reset_accum();
			}
		}
	}

	float offset() { return cal.offset(); }
	float slope() { return cal.slope(); }

private:
	void reset_accum() {
		accum = 0;
		accum_ctr = 0;
	}

	static constexpr float Tolerance = 100.f;
	bool measure(float pitch_cv, float expected) {
		accum += pitch_cv;
		if (++accum_ctr >= 32) {
			float avg = (float)accum / 32.f;
			if (std::abs(avg - expected) > Tolerance) {
				flags.set(Flag::CVCalibrationFail);
				cv_cal_step = Done;
				pr_log("Fail: avg=%f, expected=%f\n", (double)avg, (double)expected);
			} else {
				avg_cv = avg;
				return true;
			}
		}
		return false;
	}
};

} // namespace SamplerKit
