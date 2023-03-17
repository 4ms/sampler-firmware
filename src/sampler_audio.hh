#pragma once
#include "audio_stream_conf.hh"
#include "params.hh"

namespace SamplerKit
{
class SamplerAudio {
	Params &params;

public:
	SamplerAudio(Params &params)
		: params{params} {}

	void update(const AudioStreamConf::AudioInBlock &inblock, AudioStreamConf::AudioOutBlock &outblock) {
		//
	}
};

} // namespace SamplerKit
