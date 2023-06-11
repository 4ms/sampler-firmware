#pragma once
#include <array>
#include <cstdint>

namespace SamplerKit
{
enum class Flag : uint64_t {
	LatchVoltOctCV,
	RevTrig,
	PlayBut,
	PlayTrigDelaying,
	PlayTrig,
	RecBut,
	RecTrig,
	ToggleLooping,
	PlayBuffDiscontinuity,
	ForceFileReload,
	ChangePlaytoPerc,
	PlayBankChanged,
	PlaySampleChanged,
	PlaySampleChangedValid,
	PlaySampleChangedEmpty,
	PlaySampleChangedValidBright,
	PlaySampleChangedEmptyBright,

	RecStartedWithTrigger,

	EnterPlayMode,
	EnterRecordMode,

	EnterCVCalibrateMode,
	StepCVCalibration, // Buttons -> CV Cal
	CVCalibrateAllJacks,
	CVCalibrationSuccess,
	CVCalibrationFail,

	CVCalibrationStep1Animate,
	CVCalibrationStep2Animate,
	CVCalibrationSuccessAnimate,
	CVCalibrationFailAnimate,
	CVCalibrateAllJacksAnimate,

	ToggleStereoMode,
	ToggleStereoModeAnimate,

	EnterLEDCalibrateMode,

	StartFadeUp,   // env_level => 0
	StartFadeDown, // env_level => 1

	EndOutShort,
	EndOutLong,

	BankNext,
	BankPrev,
	BankReleased,

	StartupParsing,
	StartupLoadingIndex,
	StartupNewIndex,
	StartupWritingIndex,
	StartupWritingHTML,
	StartupDone,

	WriteSettingsToSD,
	WriteIndexToSD,

	NUM_FLAGS
};

struct Flags {
	using flag_base_t = std::underlying_type_t<Flag>;
	constexpr inline static auto NumFlags = static_cast<flag_base_t>(Flag::NUM_FLAGS);
	static_assert(NumFlags < sizeof(flag_base_t) * 8, "Exceeded max flags allowed");

	void set(Flag flag) { _flags |= bitnum(flag); }

	void clear(Flag flag) {
		if (_flags & bitnum(flag))
			_flags &= ~bitnum(flag);
	}

	bool read(Flag flag) { return _flags & bitnum(flag); }

	bool take(Flag flag) {
		if (_flags & bitnum(flag)) {
			_flags &= ~bitnum(flag);
			return true;
		}
		return false;
	}

private:
	flag_base_t _flags = 0;
	flag_base_t bitnum(Flag b) { return static_cast<flag_base_t>(1) << static_cast<flag_base_t>(b); }
};
} // namespace SamplerKit
