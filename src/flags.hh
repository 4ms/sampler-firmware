#pragma once
#include <array>
#include <cstdint>

namespace SamplerKit
{
enum class Flag : uint32_t {
	SampleChanged,
	SampleChangedValid,
	SampleChangedInvalid,
	LatchVoltOctCV,
	RevTrig,
	PlayBut,
	PlayTrigDelaying,
	PlayTrig,
	RecTrig,
	ToggleLooping,
	PlayBuffDiscontinuity,
	ForceFileReload,
	ChangePlaytoPerc,
	PlayBankChanged,
	PlaySampleChanged,
	PlaySampleChangedValid,
	PlaySampleChangedEmpty,
	NUM_FLAGS
};

struct Flags {
	constexpr inline static auto NumFlags = static_cast<uint32_t>(Flag::NUM_FLAGS);
	static_assert(NumFlags < 32, "Max 31 flags allowed");

	void set(Flag flag) {
		auto bit = static_cast<uint32_t>(flag);
		_flags |= (1 << bit);
	}

	void clear(Flag flag) {
		auto bit = static_cast<uint32_t>(flag);
		if (_flags & (1 << bit))
			_flags &= ~(1 << bit);
	}

	bool read(Flag flag) {
		auto bit = static_cast<uint32_t>(flag);
		return _flags & (1 << bit);
	}

	bool take(Flag flag) {
		auto bit = static_cast<uint32_t>(flag);
		if (_flags & (1 << bit)) {
			_flags &= ~(1 << bit);
			return true;
		}
		return false;
	}

private:
	uint32_t _flags = 0;
};
} // namespace SamplerKit
