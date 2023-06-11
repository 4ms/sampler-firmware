#pragma once
#include "sampler_audio.hh"
#include "sampler_loader.hh"
#include "sampler_modes.hh"
#include "wav_recording.hh"

namespace SamplerKit
{

struct Sampler {
	std::array<CircularBuffer, NumSamplesPerBank> play_buff;
	uint32_t g_error = 0;

	Sampler(Params &params, Flags &flags, Sdcard &sd, BankManager &banks)
		: audio{modes, params, flags, banks.samples, play_buff}
		, loader{modes, params, flags, sd, banks, play_buff, g_error}
		, modes{params, flags, sd, banks, recorder, play_buff, g_error}
		, recorder{params, flags, sd, banks} {}

	SamplerAudio audio;
	SampleLoader loader;
	SamplerModes modes;
	Recorder recorder;

	void start() { loader.start(); }

	void update() {
		modes.process_mode_flags();
		loader.update();
		recorder.write_buffer_to_storage();
	}
};

} // namespace SamplerKit
