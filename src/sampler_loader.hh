#pragma once
#include "bank.hh"
#include "circular_buffer.hh"
#include "flags.hh"
#include "log.hh"
#include "params.hh"
#include "sampler_modes.hh"
#include "sdcard.hh"

namespace SamplerKit
{

class SampleLoader {
	SamplerModes &s;
	Params &params;
	Flags &flags;
	Sdcard &sd;
	SampleList &samples;
	BankManager &banks;
	std::array<CircularBuffer, NumSamplesPerBank> &play_buff;
	uint32_t &g_error;

	mdrivlib::Timekeeper sdcard_update_task;
	bool time_to_update = false;

public:
	SampleLoader(SamplerModes &sampler_modes,
				 Params &params,
				 Flags &flags,
				 Sdcard &sd,
				 BankManager &banks,
				 std::array<CircularBuffer, NumSamplesPerBank> &splay_buff,
				 uint32_t &g_error)
		: s{sampler_modes}
		, params{params}
		, flags{flags}
		, sd{sd}
		, samples{banks.samples}
		, banks{banks}
		, play_buff{splay_buff}
		, g_error{g_error} {
		sdcard_update_task.init(
			{
				.TIMx = TIM7,
				.period_ns = 1'000'000'000 / 1400,
				.priority1 = 1,
				.priority2 = 0,
			},
			[this]() { time_to_update = true; });
	}

	void start() { sdcard_update_task.start(); }

	void update() {
		if (time_to_update) {
			time_to_update = false;
			read_storage_to_buffer();
		}
	}

	uint32_t tmp_buff_u32[READ_BLOCK_SIZE >> 2];

	void read_storage_to_buffer(void) {
		uint8_t chan = 0;
		uint32_t err;

		FRESULT res;
		UINT br;
		uint32_t rd;

		// convenience variables
		uint8_t samplenum, banknum;
		Sample *s_sample;
		FSIZE_t t_fptr;
		uint32_t pre_buff_size;
		uint32_t active_buff_size;
		float pb_adjustment;

		check_change_sample();
		check_change_bank();

		if ((params.play_state != PlayStates::SILENT) && (params.play_state != PlayStates::PLAY_FADEDOWN) &&
			(params.play_state != PlayStates::RETRIG_FADEDOWN))
		{
			samplenum = params.sample_num_now_playing;
			banknum = params.sample_bank_now_playing;
			s_sample = &(samples[banknum][samplenum]);

			// FixMe: Calculate play_buff_bufferedamt after play_buff changes, not here, then make bufferedmat private
			// again
			s.play_buff_bufferedamt[samplenum] = play_buff[samplenum].distance(params.reverse);

			//
			// Try to recover from a file read error
			//
			if (g_error & (FILE_READ_FAIL_1 << chan)) {
				res = reload_sample_file(&s.fil[samplenum], s_sample, sd);
				if (res != FR_OK) {
					g_error |= FILE_OPEN_FAIL;
					params.play_state = PlayStates::SILENT;
					return;
				}

				res = sd.create_linkmap(&s.fil[samplenum], samplenum);
				if (res != FR_OK) {
					g_error |= FILE_CANNOT_CREATE_CLTBL;
					f_close(&s.fil[samplenum]);
					params.play_state = PlayStates::SILENT;
					return;
				}

				// clear the error flag
				g_error &= ~(FILE_READ_FAIL_1 << chan);
			} else // If no file read error... [?? FixMe: does this logic make sense for clearing
				   // is_buffered_to_file_end ???]
			{
				if ((!params.reverse && (s.sample_file_curpos[samplenum] < s_sample->inst_end)) ||
					(params.reverse && (s.sample_file_curpos[samplenum] > s_sample->inst_start)))
					s.is_buffered_to_file_end[samplenum] = 0;
			}

			//
			// Calculate the amount to pre-buffer before we play:
			//
			pb_adjustment = params.pitch * (float)s_sample->sampleRate / (float)params.settings.record_sample_rate;

			// Calculate how many bytes we need to pre-load in our buffer
			//
			// Note of interest: blockAlign already includes numChannels, so we essentially square it in the calc
			// below. The reason is that we plow through the bytes in play_buff twice as fast if it's stereo, and
			// since it takes twice as long to load stereo data from the sd card, we have to preload four times as
			// much data (2^2) vs (1^1)
			//
			pre_buff_size = (uint32_t)((float)(BASE_BUFFER_THRESHOLD * s_sample->blockAlign * s_sample->numChannels) *
									   pb_adjustment);
			active_buff_size = pre_buff_size * 4;

			if (active_buff_size >
				((play_buff[samplenum].size * 7) / 10)) // limit amount of buffering ahead to 70% of buffer size
				active_buff_size = ((play_buff[samplenum].size * 7) / 10);

			if (!s.is_buffered_to_file_end[samplenum] && ((params.play_state == PlayStates::PREBUFFERING &&
														   (s.play_buff_bufferedamt[samplenum] < pre_buff_size)) ||
														  (params.play_state != PlayStates::PREBUFFERING &&
														   (s.play_buff_bufferedamt[samplenum] < active_buff_size))))
			{
				if (s.sample_file_curpos[samplenum] > s_sample->sampleSize) {
					// We read too much data somehow
					// TODO: When does this happen? sample_file_curpos has not changed recently...
					g_error |= FILE_WAVEFORMATERR;
					params.play_state = PlayStates::SILENT;
					s.start_playing();

				} else if (s.sample_file_curpos[samplenum] > s_sample->inst_end) {
					s.is_buffered_to_file_end[samplenum] = 1;
				}

				else
				{
					//
					// Forward reading:
					//
					if (params.reverse == 0) {
						rd = s_sample->inst_end - s.sample_file_curpos[samplenum];

						if (rd > READ_BLOCK_SIZE)
							rd = READ_BLOCK_SIZE;

						// uint32_t tmp_buff_u32;
						res = f_read(&s.fil[samplenum], (uint8_t *)tmp_buff_u32, rd, &br);
						if (res != FR_OK) {
							g_error |= FILE_READ_FAIL_1 << chan;
							s.is_buffered_to_file_end[samplenum] = 1; // FixMe: Do we really want to set this in case of
																	  // disk error? We don't when reversing.
						}

						if (br < rd) {
							g_error |= FILE_UNEXPECTEDEOF; // unexpected end of file, but we can continue writing
														   // out the data we read
							s.is_buffered_to_file_end[samplenum] = 1;
						}

						s.sample_file_curpos[samplenum] = f_tell(&s.fil[samplenum]) - s_sample->startOfData;

						if (s.sample_file_curpos[samplenum] >= s_sample->inst_end) {
							s.is_buffered_to_file_end[samplenum] = 1;
						}

					} else {
						//
						// Reverse reading:
						//
						if (s.sample_file_curpos[samplenum] > s_sample->inst_start)
							rd = s.sample_file_curpos[samplenum] - s_sample->inst_start;
						else
							rd = 0;

						if (rd >= READ_BLOCK_SIZE) {

							// Jump back a block
							rd = READ_BLOCK_SIZE;

							t_fptr = f_tell(&s.fil[samplenum]);
							res = f_lseek(&s.fil[samplenum], t_fptr - READ_BLOCK_SIZE);
							if (res || (f_tell(&s.fil[samplenum]) != (t_fptr - READ_BLOCK_SIZE)))
								g_error |= LSEEK_FPTR_MISMATCH;

							s.sample_file_curpos[samplenum] = f_tell(&s.fil[samplenum]) - s_sample->startOfData;

						} else {
							// rd < READ_BLOCK_SIZE: read the first block (which is the last to be read, since we're
							// reversing) to-do: align rd to 24

							// Jump to the beginning
							s.sample_file_curpos[samplenum] = s_sample->inst_start;
							res = s.SET_FILE_POS(banknum, samplenum);
							if (res != FR_OK)
								g_error |= FILE_SEEK_FAIL;

							s.is_buffered_to_file_end[samplenum] = 1;
						}

						// Read one block forward
						t_fptr = f_tell(&s.fil[samplenum]);
						res = f_read(&s.fil[samplenum], (uint8_t *)tmp_buff_u32, rd, &br);
						if (res != FR_OK)
							g_error |= FILE_READ_FAIL_1 << chan;

						if (br < rd)
							g_error |= FILE_UNEXPECTEDEOF;

						// Jump backwards to where we started reading
						res = f_lseek(&s.fil[samplenum], t_fptr);
						if (res != FR_OK)
							g_error |= FILE_SEEK_FAIL;
						if (f_tell(&s.fil[samplenum]) != t_fptr)
							g_error |= LSEEK_FPTR_MISMATCH;
					}

					// Write temporary buffer to play_buff[]->in
					if (res != FR_OK)
						g_error |= FILE_READ_FAIL_1 << chan;
					else {
						// Jump back in play_buff by the amount just read (re-sized from file addresses to buffer
						// address)
						if (params.reverse)
							play_buff[samplenum].offset_in_address((rd * 2) / s_sample->sampleByteSize, 1);

						err = 0;

						//
						// Write raw file data (tmp_buff_u32) into buffer (play_buff)
						//

						// 16 bit
						if (s_sample->sampleByteSize == 2)
							err = play_buff[samplenum].memory_write_16as16(tmp_buff_u32, rd >> 2, 0);

						// 24bit (rd must be a multiple of 3)
						else if (s_sample->sampleByteSize == 3)
							err = play_buff[samplenum].memory_write_24as16((uint8_t *)tmp_buff_u32, rd, 0);

						// 8bit
						else if (s_sample->sampleByteSize == 1)
							err = play_buff[samplenum].memory_write_8as16((uint8_t *)tmp_buff_u32, rd, 0);

						// 32-bit float (rd must be a multiple of 4)
						else if (s_sample->sampleByteSize == 4 && s_sample->PCM == 3)
							err = play_buff[samplenum].memory_write_32fas16((float *)tmp_buff_u32, rd >> 2, 0);

						// 32-bit int rd must be a multiple of 4
						else if (s_sample->sampleByteSize == 4 && s_sample->PCM == 1)
							err = play_buff[samplenum].memory_write_32ias16((uint8_t *)tmp_buff_u32, rd, 0);

						// Update the cache addresses
						if (params.reverse) {
							// Ignore head crossing error if we are reversing and ended up with in==out (that's
							// normal for the first reading)
							if (err && (play_buff[samplenum].in == play_buff[samplenum].out))
								err = 0;

							//
							// Jump back again in play_buff by the amount just read (re-sized from file addresses to
							// buffer address) This ensures play_buff[]->in points to the buffer seam
							//
							play_buff[samplenum].offset_in_address((rd * 2) / s_sample->sampleByteSize, 1);

							s.cache[samplenum].low = s.sample_file_curpos[samplenum];
							s.cache[samplenum].map_pt = play_buff[samplenum].in;

							if ((s.cache[samplenum].high - s.cache[samplenum].low) > s.cache[samplenum].size)
								s.cache[samplenum].high = s.cache[samplenum].low + s.cache[samplenum].size;
						} else {

							s.cache[samplenum].high = s.sample_file_curpos[samplenum];

							if ((s.cache[samplenum].high - s.cache[samplenum].low) > s.cache[samplenum].size) {
								s.cache[samplenum].map_pt = play_buff[samplenum].in;
								s.cache[samplenum].low = s.cache[samplenum].high - s.cache[samplenum].size;
							}
						}

						if (err)
							g_error |= READ_BUFF1_OVERRUN << chan;
					}
				}
			}

			// Check if we've prebuffered enough to start playing
			if ((s.is_buffered_to_file_end[samplenum] || s.play_buff_bufferedamt[samplenum] >= pre_buff_size) &&
				params.play_state == PlayStates::PREBUFFERING)
			{
				flags.set(Flag::StartFadeUp);
				//  env_level = 0.f;
				if (params.length <= 0.5f)
					params.play_state = params.reverse ? PlayStates::PLAYING_PERC : PlayStates::PERC_FADEUP;
				else
					params.play_state = PlayStates::PLAY_FADEUP;
			}

		} // params.play_state != SILENT, FADEDOWN
	}

	void check_change_bank() {
		if (flags.take(Flag::PlayBankChanged)) {

			// Set flag that the sample has changed
			flags.set(Flag::PlaySampleChanged);

			// Changing bank updates the play button "not-playing" color (dim white or dim red)
			// But avoids the bright flash of white or red by setting PlaySampleXChanged_* = 1
			//

			if (samples[params.bank][params.sample].filename[0] == 0) {
				flags.clear(Flag::PlaySampleChangedValid);
				flags.set(Flag::PlaySampleChangedEmpty);
			} else {
				flags.set(Flag::PlaySampleChangedValid);
				flags.clear(Flag::PlaySampleChangedEmpty);
			}
		}
	}

	void check_change_sample(void) {
		if (!flags.take(Flag::PlaySampleChanged))
			return;

		if (s.cached_rev_state[params.sample] != params.reverse) {
			s.reverse_file_positions(params.sample, params.bank, params.reverse);
			s.cached_rev_state[params.sample] = params.reverse;
		}

		// FixMe: Clean up this logic:
		// no file: fadedown or remain silent
		if (samples[params.bank][params.sample].filename[0] == 0) {
			// Avoid dimming it if we had already set the bright flag
			if (flags.read(Flag::PlaySampleChangedEmptyBright) == 0)
				flags.set(Flag::PlaySampleChangedEmpty);
			flags.clear(Flag::PlaySampleChangedValid);

			if (params.settings.auto_stop_on_sample_change == AutoStopMode::Always ||
				(params.settings.auto_stop_on_sample_change == AutoStopMode::Looping && params.looping))
			{
				if (params.play_state != PlayStates::SILENT && params.play_state != PlayStates::PREBUFFERING) {
					if (params.play_state == PlayStates::PLAYING_PERC)

						params.play_state = PlayStates::REV_PERC_FADEDOWN;
					else {
						params.play_state = PlayStates::PLAY_FADEDOWN;
						flags.set(Flag::StartFadeDown);
						// env_level = 1.f;
					}

				} else
					params.play_state = PlayStates::SILENT;
			}
			return;
		}

		// Sample found in this slot:

		// Avoid dimming it if we had already set the bright flag
		if (flags.read(Flag::PlaySampleChangedValidBright) == 0)
			flags.set(Flag::PlaySampleChangedValid);
		flags.clear(Flag::PlaySampleChangedEmpty);
		pr_dbg("%.80s\n", samples[params.bank][params.sample].filename);

		if (params.settings.auto_stop_on_sample_change == AutoStopMode::Always) {
			if (params.play_state == PlayStates::SILENT && params.looping)
				flags.set(Flag::PlayBut);

			if (params.play_state != PlayStates::SILENT && params.play_state != PlayStates::PREBUFFERING) {
				if (params.play_state == PlayStates::PLAYING_PERC)
					params.play_state = PlayStates::REV_PERC_FADEDOWN;
				else {
					params.play_state = PlayStates::PLAY_FADEDOWN;
					flags.set(Flag::StartFadeDown);
					// env_level = 1.f;
				}
			}
		} else {
			if (params.looping) {
				if (params.play_state == PlayStates::SILENT)
					flags.set(Flag::PlayBut);

				else if (params.settings.auto_stop_on_sample_change == AutoStopMode::Looping) {
					if (params.play_state == PlayStates::PLAYING_PERC)
						params.play_state = PlayStates::REV_PERC_FADEDOWN;
					else {
						params.play_state = PlayStates::PLAY_FADEDOWN;
						flags.set(Flag::StartFadeDown);
						// env_level = 1.f;
					}
				}
			}
		}
	}
};
} // namespace SamplerKit
