#pragma once
#include "sampler_audio.hh"
#include "sampler_loader.hh"
#include "sampler_modes.hh"
#pragma once
namespace SamplerKit
{

struct Sampler {
	std::array<CircularBuffer, NumSamplesPerBank> play_buff;
	uint32_t g_error = 0;

	Sampler(Params &params, Flags &flags, Sdcard &sd, BankManager &banks)
		: modes{params, flags, sd, banks, play_buff, g_error}
		, loader{modes, params, flags, sd, banks, play_buff, g_error}
		, audio{modes, params, flags, banks.samples, play_buff} {}

	SamplerAudio audio;
	SampleLoader loader;
	SamplerModes modes;

	void start() { loader.start(); }

	void update() {
		modes.process_mode_flags();
		loader.update();
	}
};

} // namespace SamplerKit
