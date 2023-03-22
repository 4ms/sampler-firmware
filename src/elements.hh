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

// Settings:
enum { FirmwareMajorVersion = 0 };
enum { FirmwareMinorVersion = 1 };

enum { MaxNumBanks = 60 };
enum { NumSamplesPerBank = 10 };
enum { LEDUpdateRateHz = 187 };

// 44.1k/16b/stereo@ pitch=1.0: the time from the first block read to the first sample of audio outputted:
//(This is addition to the delay from the Trigger jack to the first block being read, around 16ms)
// BASE_BUFFER_THRESHOLD/READ_BLOCK_SIZE
// 6144/16384: 13ms
// 3072/16384: 9ms
// 1536/16384: 6ms
// 6144/8192: 13ms
// 3072/8192: 9ms
// 1536/8192: 7ms

// #define BASE_BUFFER_THRESHOLD (6144)
constexpr inline uint32_t BASE_BUFFER_THRESHOLD = 3072;
// #define BASE_BUFFER_THRESHOLD (1536)

// READ_BLOCK_SIZE must be a multiple of all possible sample file block sizes
// 1(8m), 2(16m), 3(24m), 4(32m), 6(24s), 8(32s) ---> 24 is the lowest value
// It also should be a multiple of 512, since the SD Card is arranged by 512 byte sectors
// 9216 = 512 * 18 = 24 * 384
constexpr inline uint32_t READ_BLOCK_SIZE = 9216;
constexpr inline float PERC_ENV_FACTOR = 40000.0f;
constexpr inline float MAX_RS = 20.f;
} // namespace SamplerKit
