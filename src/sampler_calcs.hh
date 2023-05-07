#pragma once
#include "audio_stream_conf.hh"
#include "circular_buffer.hh"
#include "elements.hh"
#include "sample_file.hh"

namespace SamplerKit
{

constexpr uint32_t FramesPerBlock = AudioStreamConf::BlockSize; // TODO: Check this is right?

inline uint32_t align_addr(uint32_t addr, uint32_t blockAlign) {
	uint32_t t;

	if (blockAlign == 4)
		addr &= 0xFFFFFFFC;
	else if (blockAlign == 2)
		addr &= 0xFFFFFFF8; // STS: was E? but it clicks if we align to 2 not 4, even if our file claims blockAlign = 2
	else if (blockAlign == 8)
		addr &= 0xFFFFFFF8;
	else if (blockAlign == 6) {
		t = addr / 6UL;
		addr = t * 6UL;
	} else if (blockAlign == 3) {
		t = addr / 3UL;
		addr = t * 3UL;
	} else if (blockAlign == 1)
		return addr;
	else
		return 0; // unknown block Align

	return addr;
}

// Returns number of blocks that we should take to do a fast fade in perc mode
// Fast fades are PERC_FADEUP and REV_PERC_FADEDOWN
// and when we need to fade-down quickle because we're about to hit the end
inline uint32_t calc_perc_fadedown_blocks(float length, float sample_rate) {
	const float time = length * PERC_ENV_FACTOR / sample_rate;

	if (time <= 0.010f) // faster than 100Hz, 10ms --> 1 block (0.3ms at 44k)
		return 1;

	const uint32_t max_fadedown = (0.003f * sample_rate) / FramesPerBlock;

	if (time >= 0.100f) // slower than 10Hz, 100ms --> 3ms fade
		return max_fadedown;

	return ((time - 0.010f) / 0.100f) * (max_fadedown - 1) + 1;
	// TODO: compare performance of this vs. doing the interpolate and then limiting
	// TODO: compare performance of this vs. pre-calculating PERC_ENV_FACTOR/sr and max_fadedown whenever sr changes
	// 44100Hz --> 3ms is 8.2 blocks, so return 9 blocks
}

inline float calc_fast_perc_fade_rate(float length, float sample_rate) {
	return 1.f / (calc_perc_fadedown_blocks(length, sample_rate) * FramesPerBlock);
}

// calc_resampled_cache_size()
// Amount an imaginary pointer in the sample file would move with each audio block sent to the codec
inline uint32_t calc_resampled_cache_size(const Sample &sample, uint32_t resampled_buffer_size) {
	return ((resampled_buffer_size * sample.sampleByteSize) / 2);
}

// calc_resampled_buffer_size()
// Amount play_buff[]->out changes with each audio block sent to the codec
inline uint32_t calc_resampled_buffer_size(const Sample &sample, float resample_rate) {
	return ((uint32_t)((FramesPerBlock * sample.numChannels * 2) * resample_rate));
}

// calc_start_point()
inline uint32_t calc_start_point(float start_param, Sample *const sample) {
	uint32_t zeropt;
	uint32_t inst_size;

	zeropt = sample->inst_start;
	inst_size = sample->inst_end - sample->inst_start;

	// If the sample size is smaller than two blocks, the start point is forced to the start
	if (inst_size <= (READ_BLOCK_SIZE * 2))
		return (align_addr(zeropt, sample->blockAlign));

	if (start_param < 0.002f)
		return (align_addr(zeropt, sample->blockAlign));

	else if (sample->num_cues) {
		return 0;
	} else if (start_param > 0.998f)
		// just play the last 32 blocks (~64k samples)
		return (align_addr((zeropt + inst_size - (READ_BLOCK_SIZE * 2)), sample->blockAlign));

	else
		return (align_addr((zeropt + ((uint32_t)(start_param * (float)inst_size))), sample->blockAlign));
}

inline uint32_t ceil(float num) {
	uint32_t inum = (uint32_t)num;
	if (num == (float)inum) {
		return inum;
	}
	return inum + 1;
}

// calc_stop_point()
// Returns an offset from the startpos, based on the length param (knob and CV) and resampling rate
//
static uint32_t
calc_stop_point(float length_param, float resample_param, Sample *sample, uint32_t startpos, float sample_rate) {
	uint32_t fwd_stop_point;
	uint32_t num_samples_to_play;
	uint32_t max_play_length;
	float seconds;
	float t_f;
	uint32_t t_int;

	seconds = (float)(sample->sampleRate * sample->blockAlign);
	max_play_length = sample->inst_end - sample->inst_start;
	// as opposed to taking sample->inst_size because that won't be clipped to the end of a sample file

	// 100% >= knob > 98% -->  play 100%
	if (length_param > 0.98f) {
		num_samples_to_play = max_play_length;
	}

	// 98% >= knob >= 50% and sample length <= 0.625s
	else if (length_param > 0.5f && max_play_length <= (0.625f * seconds))
	{
		const float half_sec = 0.5f * seconds;
		// --- 0 < sample length <= 0.5s ---> play full sample length
		if (max_play_length <= half_sec)
			num_samples_to_play = max_play_length;
		else
			// --- 0.5s < sample length <= 0.625 ---> play between 0.5s and full sample
			num_samples_to_play = half_sec + ((max_play_length - half_sec) * (length_param - 0.5f) / (0.98f - 0.5f));
	}

	// 98% >= knob > 95%  --> play between 100% to 80% of full sample
	else if (length_param > 0.95f)
	{
		num_samples_to_play = max_play_length * (6.67f * length_param - 5.5366f);
	}

	// 95% >= knob > 50%  (and sample length > 0.625s)
	else if (length_param > 0.50f)
	{
		// --- sample length > 5s
		if (max_play_length > (5.0f * seconds)) {
			if (length_param > 0.85f)
				// -------- 95% >= knob > 85%  --> play 80%  to 5.5s
				num_samples_to_play =
					4.5f * seconds + ((length_param - 0.85f) * 8.f) * (max_play_length - 4.5f * seconds);
			else
				// -------- 85% >= knob > 50%  --> play 4.5s to 0.5s
				num_samples_to_play = (11.43f * length_param - 5.215f) * seconds;
		}

		// --- 5s >= sample length > 0.625s --> play 0.5s to 80%
		else
		{
			num_samples_to_play =
				0.5f * seconds + (length_param * 1.7778f - 0.8889f) * (max_play_length - 0.625f * seconds);
		}
	}

	// length_param <= 50%: fixed envelope
	else
	{
		// number of sample frames to play is length*PERC_ENV_FACTOR plus the fadedown env
		// rounded up to multiples of FramesPerBlock
		// times the playback resample rate (ratio of wavfile SR : playback SR)
		// times the block align gives us how much the buffer address will increment
		t_f = (length_param * PERC_ENV_FACTOR) + calc_perc_fadedown_blocks(length_param, sample_rate);

		// round up to nearest audio block size
		t_f = ceil(t_f / FramesPerBlock) * FramesPerBlock;

		t_f *= resample_param;
		t_int = ((uint32_t)t_f) * sample->blockAlign;
		num_samples_to_play = t_int;
	}

	fwd_stop_point = startpos + num_samples_to_play;

	if (fwd_stop_point > sample->inst_end)
		fwd_stop_point = sample->inst_end;

	return align_addr(fwd_stop_point, sample->blockAlign);
}

} // namespace SamplerKit
