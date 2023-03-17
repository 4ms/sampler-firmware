#pragma once
#include "audio_memory.hh"
#include "audio_stream_conf.hh"
#include "cache.hh"
#include "circular_buffer.hh"
#include "errors.hh"
#include "flags.hh"
#include "params.hh"
#include "sampler_calcs.hh"
#include "sdcard.hh"

namespace SamplerKit
{

class Sampler {
	static constexpr uint32_t NUM_SAMPLES_PER_BANK = NumSamplesPerBank;

	Params &params;
	Flags &flags;
	Sdcard &sd;

	FIL fil[NumSamplesPerBank];
	uint32_t g_error = 0;

	static const inline uint32_t PLAY_BUFF_START = Board::MemoryStartAddr;
	static constexpr uint32_t PLAY_BUFF_SLOT_SIZE = 0x0012'0000; // 1.15MB
	// TODO: get rid of ptr
	CircularBuffer splay_buff[NumSamplesPerBank];
	CircularBuffer *play_buff[NumSamplesPerBank];

	enum PlayStates {
		SILENT,
		PREBUFFERING,
		PLAY_FADEUP,
		PERC_FADEUP,
		PLAYING,
		PLAYING_PERC,
		PLAY_FADEDOWN,
		RETRIG_FADEDOWN,
		REV_PERC_FADEDOWN,
		PAD_SILENCE,
	} play_state;

	uint8_t sample_num_now_playing;
	uint8_t sample_bank_now_playing;

	Cache cache[NumSamplesPerBank];

	// 1 = file is totally cached (from inst_start to inst_end), otherwise 0
	bool is_buffered_to_file_end[NumSamplesPerBank];
	uint32_t play_buff_bufferedamt[NumSamplesPerBank];
	bool cached_rev_state[NumSamplesPerBank];

	// file position where we began playback.
	uint32_t sample_file_startpos;
	// file position where we will end playback. endpos > startpos when REV==0, endpos < startpos when REV==1
	uint32_t sample_file_endpos;
	// current file position being read. Must match the actual open file's position. This is always inc/decrementing
	// from startpos towards endpos
	uint32_t sample_file_curpos[NumSamplesPerBank];

	float env_level;
	float env_rate = 0.f;

	uint32_t last_play_start_tmr;

public:
	Sampler(Params &params, Flags &flags, Sdcard &sd)
		: params{params}
		, flags{flags}
		, sd{sd} {

		Memory::clear();
		// TODO: init_recbuff();
		for (unsigned i = 0; i < NUM_SAMPLES_PER_BANK; i++) {
			play_buff[i] = &(splay_buff[i]);

			play_buff[i]->min = PLAY_BUFF_START + (i * PLAY_BUFF_SLOT_SIZE);
			play_buff[i]->max = play_buff[i]->min + PLAY_BUFF_SLOT_SIZE;
			play_buff[i]->size = PLAY_BUFF_SLOT_SIZE;

			play_buff[i]->in = play_buff[i]->min;
			play_buff[i]->out = play_buff[i]->min;

			play_buff[i]->wrapping = 0;

			cache[i].map_pt = play_buff[i]->min;
			cache[i].low = 0;
			cache[i].high = 0;
			cached_rev_state[i] = 0;
			play_buff_bufferedamt[i] = 0;
			is_buffered_to_file_end[i] = 0;
		}

		flags.set(Flag::PlayBuffDiscontinuity);
	}

	void process_mode_flags() {
		if (flags.take(Flag::RevTrig))
			toggle_reverse();

		if (flags.take(Flag::PlayBut))
			toggle_playing();

		if (flags.read(Flag::PlayTrigDelaying)) {
			uint32_t time_since_play_trig = HAL_GetTick() - params.play_trig_timestamp;
			if (time_since_play_trig > params.settings.play_trig_latch_pitch_time)
				flags.clear(Flag::LatchVoltOctCV);
			else
				flags.set(Flag::LatchVoltOctCV);

			if (time_since_play_trig > params.settings.play_trig_delay) {
				flags.set(Flag::PlayTrig);
				flags.clear(Flag::PlayTrigDelaying);
				flags.clear(Flag::LatchVoltOctCV);
			}
		}

		if (flags.take(Flag::PlayTrig))
			start_restart_playing();

		if (flags.take(Flag::RevTrig))
			toggle_recording();

		if (flags.take(Flag::ToggleLooping)) {
			params.looping = !params.looping;
			if (params.looping && play_state == SILENT)
				// STS: was flags.set(Flag::PlayBut);
				toggle_playing();
		}
	}

private:
	void toggle_reverse() {
		uint8_t samplenum, banknum;

		if (play_state == PLAYING || play_state == PLAYING_PERC || play_state == PREBUFFERING ||
			play_state == PLAY_FADEUP || play_state == PERC_FADEUP)
		{
			samplenum = sample_num_now_playing;
			banknum = sample_bank_now_playing;
		} else {
			samplenum = params.sample;
			banknum = params.bank;
		}

		// Prevent issues if playback interrupts this routine
		PlayStates tplay_state = play_state;
		play_state = SILENT;

		// If we are PREBUFFERING or PLAY_FADEUP, then that means we just started playing.
		// It could be the case a common trigger fired into PLAY and REV, but the PLAY trig was detected first
		// So we actually want to make it play from the end of the sample rather than reverse direction from the current
		// spot
		if (tplay_state == PREBUFFERING || tplay_state == PLAY_FADEUP || tplay_state == PLAYING ||
			tplay_state == PERC_FADEUP)
		{
			// Handle a rev trig shortly after a play trig by playing as if the rev trig was first
			if ((HAL_GetTick() - last_play_start_tmr) < (params.settings.record_sample_rate * 0.1f)) // 100ms
			{
				// See if the endpos is within the cache, then we can just play from that point
				if ((sample_file_endpos >= cache[samplenum].low) && (sample_file_endpos <= cache[samplenum].high)) {
					play_buff[samplenum]->out = cache[samplenum].map_cache_to_buffer(
						sample_file_endpos, params.samples[banknum][samplenum].sampleByteSize, play_buff[samplenum]);
				} else {
					// Otherwise we have to make a new cache, so run start_playing()
					params.reverse = !params.reverse;
					start_playing();
					return;
				}
			}
		}
		params.reverse = !params.reverse;
		reverse_file_positions(samplenum, banknum, params.reverse);
		cached_rev_state[params.sample] = params.reverse;

		// Restore play_state
		play_state = tplay_state;
	}

	void toggle_playing() {
		//
	}

	void start_restart_playing() {
		//
	}

	void toggle_recording() {
		//
	}

	void start_playing() {
		FRESULT res;
		float rs;

		uint8_t samplenum = params.sample;
		uint8_t banknum = params.bank;
		Sample *s_sample = &(params.samples[banknum][samplenum]);

		if (s_sample->filename[0] == 0)
			return;

		sample_num_now_playing = samplenum;

		if (banknum != sample_bank_now_playing) {
			sample_bank_now_playing = banknum;
			init_changed_bank();
		}

		// Reload the sample file if necessary:
		// Force Reload flag is set (Edit mode, or loaded new index)
		// File is empty (never been read since entering this bank)
		// Sample File Changed flag is set (new file was recorded into this slot)
		if (flags.take(Flag::ForceFileReload) || (fil[samplenum].obj.fs == 0)) {
			res = reload_sample_file(&fil[samplenum], s_sample, sd);
			if (res != FR_OK) {
				g_error |= FILE_OPEN_FAIL;
				play_state = SILENT;
				return;
			}

			res = sd.create_linkmap(&fil[samplenum], samplenum);
			if (res == FR_NOT_ENOUGH_CORE) {
				g_error |= FILE_CANNOT_CREATE_CLTBL;
			} // ToDo: Log this error
			else if (res != FR_OK)
			{
				g_error |= FILE_CANNOT_CREATE_CLTBL;
				f_close(&fil[samplenum]);
				play_state = SILENT;
				return;
			}

			// Check the file is really as long as the sampleSize says it is
			if (f_size(&fil[samplenum]) < (s_sample->startOfData + s_sample->sampleSize)) {
				s_sample->sampleSize = f_size(&fil[samplenum]) - s_sample->startOfData;

				if (s_sample->inst_end > s_sample->sampleSize)
					s_sample->inst_end = s_sample->sampleSize;

				if ((s_sample->inst_start + s_sample->inst_size) > s_sample->sampleSize)
					s_sample->inst_size = s_sample->sampleSize - s_sample->inst_start;
			}

			cache[samplenum].low = 0;
			cache[samplenum].high = 0;
			cache[samplenum].map_pt = play_buff[samplenum]->min;
		}

		// Calculate our actual resampling rate
		rs = params.pitch * ((float)s_sample->sampleRate / params.settings.record_sample_rate);

		// Determine starting and ending addresses
		if (params.reverse) {
			sample_file_endpos = calc_start_point(params.start, s_sample);
			sample_file_startpos =
				calc_stop_point(params.length, rs, s_sample, sample_file_endpos, params.settings.record_sample_rate);
		} else {
			sample_file_startpos = calc_start_point(params.start, s_sample);
			sample_file_endpos =
				calc_stop_point(params.length, rs, s_sample, sample_file_startpos, params.settings.record_sample_rate);
		}

		// See if the starting position is already cached
		if ((cache[samplenum].high > cache[samplenum].low) && (cache[samplenum].low <= sample_file_startpos) &&
			(sample_file_startpos <= cache[samplenum].high))
		{
			play_buff[samplenum]->out = cache[samplenum].map_cache_to_buffer(
				sample_file_startpos, s_sample->sampleByteSize, play_buff[samplenum]);

			env_level = 0.f;
			if (params.length <= 0.5f)
				play_state = params.reverse ? PLAYING_PERC : PERC_FADEUP;
			else
				play_state = PLAY_FADEUP;

		} else {
			//...otherwise, start buffering from scratch
			// Set state to silent so we don't run play_audio_buffer(), which could result in a glitch since the
			// playbuff and cache values are being changed
			play_state = SILENT;
			play_buff[samplenum]->init();

			// Seek to the file position where we will start reading
			sample_file_curpos[samplenum] = sample_file_startpos;
			res = SET_FILE_POS(banknum, samplenum);

			// If seeking fails, perhaps we need to reload the file
			if (res != FR_OK) {
				res = reload_sample_file(&fil[samplenum], s_sample, sd);
				if (res != FR_OK) {
					g_error |= FILE_OPEN_FAIL;
					play_state = SILENT;
					return;
				}

				res = sd.create_linkmap(&fil[samplenum], samplenum);
				if (res != FR_OK) {
					g_error |= FILE_CANNOT_CREATE_CLTBL;
					f_close(&fil[samplenum]);
					play_state = SILENT;
					return;
				}

				res = SET_FILE_POS(banknum, samplenum);
				if (res != FR_OK) {
					g_error |= FILE_SEEK_FAIL;
				}
			}
			if (g_error & LSEEK_FPTR_MISMATCH) {
				sample_file_startpos =
					align_addr(f_tell(&fil[samplenum]) - s_sample->startOfData, s_sample->blockAlign);
			}

			cache[samplenum].low = sample_file_startpos;
			cache[samplenum].high = sample_file_startpos;
			cache[samplenum].map_pt = play_buff[samplenum]->min;
			cache[samplenum].size = (play_buff[samplenum]->size >> 1) * s_sample->sampleByteSize;
			is_buffered_to_file_end[samplenum] = 0;

			play_state = PREBUFFERING;
		}

		// used by toggle_reverse() to see if we hit a reverse trigger right after a play trigger
		last_play_start_tmr = HAL_GetTick();

		flags.set(Flag::PlayBuffDiscontinuity);

		env_level = 0.f;
		env_rate = 0.f;

		// play_led_state = 1;

#ifdef DEBUG_ENABLED
		str_cpy(dbg_sample.filename, s_sample->filename);
		dbg_sample.sampleSize = s_sample->sampleSize;
		dbg_sample.sampleByteSize = s_sample->sampleByteSize;
		dbg_sample.sampleRate = s_sample->sampleRate;
		dbg_sample.numChannels = s_sample->numChannels;
		dbg_sample.startOfData = s_sample->startOfData;
		dbg_sample.blockAlign = s_sample->blockAlign;
		dbg_sample.PCM = s_sample->PCM;
		dbg_sample.inst_start = s_sample->inst_start;
		dbg_sample.inst_end = s_sample->inst_end;
		dbg_sample.inst_size = s_sample->inst_size;
		dbg_sample.inst_gain = s_sample->inst_gain;
#endif
	}

	void reverse_file_positions(uint8_t samplenum, uint8_t banknum, bool new_dir) {
		// Swap sample_file_curpos with cache_high or _low
		// and move ->in to the equivalant address in play_buff
		// This gets us ready to read new data to the opposite end of the cache.
		if (new_dir) {
			sample_file_curpos[samplenum] = cache[samplenum].low;
			play_buff[samplenum]->in = cache[samplenum].map_pt; // cache_map_pt is the map of cache_low
		} else {
			sample_file_curpos[samplenum] = cache[samplenum].high;
			play_buff[samplenum]->in = cache[samplenum].map_cache_to_buffer(
				cache[samplenum].high, params.samples[banknum][samplenum].sampleByteSize, play_buff[samplenum]);
		}

		// Swap the endpos with the startpos
		// This way, curpos is always moving towards endpos and away from startpos
		std::swap(sample_file_endpos, sample_file_startpos);

		// Seek the starting position in the file
		// This gets us ready to start reading from the new position
		if (fil[samplenum].obj.id > 0) {
			FRESULT res;
			res = SET_FILE_POS(banknum, samplenum);
			if (res != FR_OK)
				g_error |= FILE_SEEK_FAIL;
		}
	}

	void init_changed_bank() {
		uint8_t samplenum;
		FRESULT res;

		for (samplenum = 0; samplenum < NUM_SAMPLES_PER_BANK; samplenum++) {
			res = f_close(&fil[samplenum]);
			if (res != FR_OK)
				fil[samplenum].obj.fs = 0;

			is_buffered_to_file_end[samplenum] = 0;

			play_buff[samplenum]->init();
		}
	}

	FRESULT SET_FILE_POS(uint8_t b, uint8_t s) {
		FRESULT r = f_lseek(&fil[s], params.samples[b][s].startOfData + sample_file_curpos[s]);
		if (fil[s].fptr != (params.samples[b][s].startOfData + sample_file_curpos[s]))
			g_error |= LSEEK_FPTR_MISMATCH;
		return r;
	}
};
} // namespace SamplerKit
