#pragma once

#include <cstdint>

namespace SamplerKit
{

enum PotAdcElement : uint32_t {
	PitchPot,
	StartPot,
	LengthPot,
	SamplePot,
};

enum CVAdcElement : uint32_t {
	PitchCV,
	StartCV,
	LengthCV,
	SampleCV,
	BankCV,
};

constexpr static uint32_t NumPots = 4;
constexpr static uint32_t NumCVs = 5;
constexpr static uint32_t NumAdcs = NumPots + NumCVs;

enum TrigInJackElement : uint32_t { PlayRecJack, RevJack };

enum TrigOutElement : uint32_t { EndOutJack };

} // namespace SamplerKit
