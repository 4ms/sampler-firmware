#pragma once
#include "audio_stream_conf.hh"
#include "circular_buffer.hh"
#include "params.hh"
#include "resample.hh"
#include "sampler.hh"
#include "sampler_calcs.hh"
#include "ssat.h"
#include "util/zip.hh"

namespace SamplerKit
{

class SamplerAudio {
	Params &params;
	SampleList &samples;
	Flags &flags;
	CircularBuffer *play_buff[NumSamplesPerBank];
	SamplerModes &sampler_modes;

	using ChanBuff = std::array<AudioStreamConf::SampleT, AudioStreamConf::BlockSize>;

public:
	float env_level;
	float env_rate = 0.f;

	SamplerAudio(Params &params,
				 SampleList &samples,
				 Flags &flags,
				 std::array<CircularBuffer, NumSamplesPerBank> &splay_buff,
				 SamplerModes &sampler_modes)
		: params{params}
		, samples{samples}
		, flags{flags}
		, sampler_modes{sampler_modes} {
		for (unsigned i = 0; i < NumSamplesPerBank; i++)
			play_buff[i] = &(splay_buff[i]);
	}

	void update(const AudioStreamConf::AudioInBlock &inblock, AudioStreamConf::AudioOutBlock &outblock) {
		// record_audio_to_buffer(inblock);
		ChanBuff outL;
		ChanBuff outR;

		play_audio_from_buffer(outL, outR);

		if (params.op_mode == OperationMode::Calibrate) {
			// Calibrate mode: output a null signal plus the calibration offset
			// This allows us to use a scope or meter while finely adjusting the
			// calibration value, until we get a DC level of zero volts on the
			// audio outut jacks.
			for (auto [L, R] : zip(outL, outR)) {
				L = params.system_calibrations.codec_dac_calibration_dcoffset[0];
				R = params.system_calibrations.codec_dac_calibration_dcoffset[1];
			}
			return;
		}

		if (params.settings.stereo_mode) {
			// Stereo mode
			// Left Out = Left Sample channel
			// Right Out = Right Sample channel
			//
			for (auto [out, L, R] : zip(outblock, outL, outR)) {
				// Chan 1 L + Chan 2 L clipped at signed 16-bits
				out.chan[0] = SSAT16(L + params.system_calibrations.codec_dac_calibration_dcoffset[0]);
				out.chan[1] = SSAT16(R + params.system_calibrations.codec_dac_calibration_dcoffset[1]);
			}
			return;
		}

		{
			// Mono mode
			// Left Out = Right Out = average of L+R
			for (auto [out, L, R] : zip(outblock, outL, outR)) {
				// Average is already done in play_audio_from_buffer(), and put into outL
				int32_t t = L;
				out.chan[0] = SSAT16(t + params.system_calibrations.codec_dac_calibration_dcoffset[0]);
				out.chan[1] = SSAT16(t + params.system_calibrations.codec_dac_calibration_dcoffset[1]);
			}
		}
	}

	void play_audio_from_buffer(ChanBuff &outL, ChanBuff &outR) {

		// Fill buffer with silence and return if we're not playing
		if (sampler_modes.play_state == SamplerModes::PREBUFFERING || sampler_modes.play_state == SamplerModes::SILENT)
		{
			for (auto [L, R] : zip(outL, outR)) {
				L = 0;
				R = 0;
			}
			return;
		}

		// Calculate our actual resampling rate, based on the sample rate of the file being played
		uint8_t samplenum = params.sample_num_now_playing;
		uint8_t banknum = params.sample_bank_now_playing;
		Sample &s_sample = samples[banknum][samplenum];

		float rs = (s_sample.sampleRate == params.settings.record_sample_rate) ?
					   params.pitch :
					   params.pitch * ((float)s_sample.sampleRate / (float)params.settings.record_sample_rate);

		sampler_modes.check_sample_end();

		bool flush = flags.read(Flag::PlayBuffDiscontinuity);
		if (params.settings.stereo_mode) {
			if ((rs * s_sample.numChannels) > MAX_RS)
				rs = MAX_RS / (float)s_sample.numChannels;

			if (s_sample.numChannels == 2) {
				uint32_t t_u32 = play_buff[samplenum]->out;
				resample_read16_left(rs, play_buff[samplenum], outL.size(), 4, outL.data(), params.reverse, flush);

				play_buff[samplenum]->out = t_u32;
				resample_read16_right(rs, play_buff[samplenum], outR.size(), 4, outR.data(), params.reverse, flush);

			} else {
				// MONO: read left channel and copy to right
				bool flush = flags.read(Flag::PlayBuffDiscontinuity);
				resample_read16_left(rs, play_buff[samplenum], outL.size(), 2, outL.data(), params.reverse, flush);
				for (unsigned i = 0; i < outL.size(); i++)
					outR[i] = outL[i];
			}
		} else { // not STEREO_MODE:
			if (rs > MAX_RS)
				rs = MAX_RS;

			if (s_sample.numChannels == 2)
				resample_read16_avg(rs, play_buff[samplenum], outL.size(), 4, outL.data(), params.reverse, flush);
			else
				resample_read16_left(rs, play_buff[samplenum], outL.size(), 2, outL.data(), params.reverse, flush);
		}

		// TODO: if writing a flag gets expensive, then we could refactor this
		// The only purpose of this flag is to set flush=true when
		//  - loading A new sample, or
		//  - When rs goes from ==1 to !=1
		// Note that flush is ignored in resample_read when rs==1
		if (rs == 1.f)
			flags.set(Flag::PlayBuffDiscontinuity);
		else
			flags.clear(Flag::PlayBuffDiscontinuity);

		apply_envelopes(outL, outR);
	}

	// Linear fade of stereo data in outL and outR
	// Gain is a fixed gain to apply to all samples
	// Set rate to < 0 to fade down, > 0 to fade up
	// Returns amplitude applied to the last sample
	// Note: this increments amplitude before applying to the first sample
	static float fade(ChanBuff &outL, ChanBuff &outR, float gain, float starting_amp, float rate) {
		float amp = starting_amp;
		uint32_t completed = 0;

		for (int i = 0; i < outL.size(); i++) {
			amp += rate;
			if (amp >= 1.0f)
				amp = 1.0f;
			if (amp <= 0.f)
				amp = 0.f;
			outL[i] = (float)outL[i] * amp * gain;
			outR[i] = (float)outR[i] * amp * gain;
			outL[i] = SSAT16(outL[i]);
			outR[i] = SSAT16(outR[i]);
		}
		return amp;
	}

	static void apply_gain(ChanBuff &outL, ChanBuff &outR, float gain) {
		for (int i = 0; i < outL.size(); i++) {
			outL[i] = (float)outL[i] * gain;
			outR[i] = (float)outR[i] * gain;
			outL[i] = SSAT16(outL[i]);
			outR[i] = SSAT16(outR[i]);
		}
	}

	void apply_envelopes(ChanBuff &outL, ChanBuff &outR) {
		int i;
		float env;

		uint8_t samplenum = params.sample_num_now_playing;
		uint8_t banknum = params.sample_bank_now_playing;
		Sample *s_sample = &(samples[banknum][samplenum]);

		float length = params.length;
		float gain = s_sample->inst_gain * params.volume;
		float rs = (s_sample->sampleRate == params.settings.record_sample_rate) ?
					   params.pitch :
					   params.pitch * ((float)s_sample->sampleRate / (float)params.settings.record_sample_rate);

		// Update the start/endpos based on the length parameter
		// Update the play_time (used to calculate led flicker and END OUT pulse width
		// ToDo: we should do this in update_params

		float play_time;
		if (params.reverse) {
			sampler_modes.sample_file_startpos = calc_stop_point(
				length, rs, s_sample, sampler_modes.sample_file_endpos, (float)params.settings.record_sample_rate);
			play_time = (sampler_modes.sample_file_startpos - sampler_modes.sample_file_endpos) /
						(s_sample->blockAlign * s_sample->sampleRate * params.pitch);
		} else {
			sampler_modes.sample_file_endpos = calc_stop_point(
				length, rs, s_sample, sampler_modes.sample_file_startpos, (float)params.settings.record_sample_rate);
			play_time = (sampler_modes.sample_file_endpos - sampler_modes.sample_file_startpos) /
						(s_sample->blockAlign * s_sample->sampleRate * params.pitch);
		}

		const float fast_perc_fade_rate = calc_fast_perc_fade_rate(length, (float)params.settings.record_sample_rate);

		// retrig fadedown rate is the faster of perc fade and global non-perc fadedown rate (larger rate == faster
		// fade)
		const float fast_retrig_fade_rate = (fast_perc_fade_rate < params.settings.fade_down_rate) ?
												params.settings.fade_down_rate :
												fast_perc_fade_rate;

		switch (sampler_modes.play_state) {

			case (SamplerModes::RETRIG_FADEDOWN):
				// DEBUG0_ON;
				env_rate =
					params.settings.fadeupdown_env ? fast_retrig_fade_rate : (1.0f / (float)AudioStreamConf::BlockSize);
				env_level = fade(outL, outR, gain, env_level, -1.f * env_rate);
				flicker_endout(play_time);

				if (env_level <= 0.f) {
					env_level = 0.f;
					// Start playing again unless we faded down because of a play trigger
					// TODO: Does this ever happen?
					if (!flags.read(Flag::PlayTrigDelaying))
						flags.set(Flag::PlayBut);

					sampler_modes.play_state = SamplerModes::SILENT;
				}
				break;

			case (SamplerModes::PLAY_FADEUP):
				if (params.settings.fadeupdown_env) {
					env_rate = params.settings.fade_up_rate;
					env_level = fade(outL, outR, gain, env_level, env_rate);
					if (env_level >= 1.f)
						sampler_modes.play_state = SamplerModes::PLAYING;

				} else {
					apply_gain(outL, outR, gain);
					sampler_modes.play_state = SamplerModes::PLAYING;
				}
				break;

			case (SamplerModes::PERC_FADEUP):
				env_rate = fast_perc_fade_rate;
				if (params.settings.perc_env) {
					env_level = fade(outL, outR, gain, env_level, env_rate);
				} else {
					// same rate as fadeing, but don't apply the envelope
					apply_gain(outL, outR, gain);
					env_level += env_rate * AudioStreamConf::BlockSize;
				}
				if (env_level >= 1.f) {
					sampler_modes.play_state = SamplerModes::PLAYING_PERC;
					env_level = 1.f;
				}
				break;

			case (SamplerModes::PLAYING):
				apply_gain(outL, outR, gain);
				if (length <= 0.5f)
					flags.set(Flag::ChangePlaytoPerc);
				break;

			case (SamplerModes::PLAY_FADEDOWN):
				if (params.settings.fadeupdown_env) {
					env_rate = params.settings.fade_down_rate;
					env_level = fade(outL, outR, gain, env_level, -1.f * env_rate);
				} else {
					apply_gain(outL, outR, gain);
					env_level = 0.f; // set this so we detect "end of fade" in the next block
				}

				if (env_level <= 0.f) {
					flicker_endout(play_time);

					// Start playing again if we're looping, unless we faded down because of a play trigger
					if (params.looping && !flags.read(Flag::PlayTrigDelaying))
						flags.set(Flag::PlayBut);

					sampler_modes.play_state = SamplerModes::SILENT;
				}
				break;

			case (SamplerModes::PLAYING_PERC):
				env_rate = (params.reverse ? 1.f : -1.f) / (length * PERC_ENV_FACTOR);
				if (params.settings.perc_env) {
					env_level = fade(outL, outR, gain, env_level, env_rate);
				} else {
					// Calculate the envelope in order to keep the timing the same vs. PERC_ENVELOPE enabled,
					// but just don't apply the envelope
					apply_gain(outL, outR, gain);
					env_level += env_rate * AudioStreamConf::BlockSize;
				}

				// After fading up to full amplitude in a reverse percussive playback, fade back down to silence:
				if ((env_level >= 1.0f) && (params.reverse)) {
					sampler_modes.play_state = SamplerModes::REV_PERC_FADEDOWN;
					env_level = 1.f;
				} else
					check_perc_ending();
				break;

			case (SamplerModes::REV_PERC_FADEDOWN):
				// Fade down to silence before going to PAD_SILENCE mode or ending the playback
				// (this prevents a click if the sample data itself doesn't cleanly fade out)
				env_rate = -1.f * fast_perc_fade_rate;
				if (params.settings.perc_env) {
					env_level = fade(outL, outR, gain, env_level, env_rate);
				} else {
					apply_gain(outL, outR, gain);
					env_level += env_rate * AudioStreamConf::BlockSize;
				}

				// If the end point is the end of the sample data (which happens if the file is very short, or if we're
				// at the end of it) Then pad it with silence so we keep a constant End Out period when looping
				if (env_level <= 0.f && sampler_modes.sample_file_endpos == s_sample->inst_end)
					sampler_modes.play_state = SamplerModes::PAD_SILENCE;
				else
					check_perc_ending();
				break;

			case (SamplerModes::PAD_SILENCE):
				for (i = 0; i < AudioStreamConf::BlockSize; i++) {
					outL[i] = 0;
					outR[i] = 0;
				}
				check_perc_ending();
				break;

			default: // PREBUFFERING or SILENT
				break;

		} // switch sampler_modes.play_state
	}

	void check_perc_ending() {
		// End the playback, go to silence, and trigger another play if looping
		if (env_level <= 0.0f || env_level >= 1.0f) {
			env_level = 0.0f;

			flicker_endout(params.length * 3.0f);

			// Restart loop
			if (params.looping && !flags.read(Flag::PlayTrigDelaying))
				flags.set(Flag::PlayTrig);

			sampler_modes.play_state = SamplerModes::SILENT;
		}
	}

	void flicker_endout(float tm) {
		// TODO: how to connct to EndOut jack?
	}
};

} // namespace SamplerKit
