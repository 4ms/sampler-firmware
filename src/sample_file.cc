/*
 * sample_file.c - Handles filesystem stuff around sample (wav) files
 *
 * Author: Dan Green (danngreen1@gmail.com)
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 * See http://creativecommons.org/licenses/MIT/ for more information.
 *
 * -----------------------------------------------------------------------------
 */
#include "sample_file.hh"
#include "bank_util.hh"
#include "errors.hh"
#include "ff.h"
#include "sdcard.hh"
#include "str_util.h"
#include "sts_filesystem.hh"
#include "wavefmt.hh"

namespace SamplerKit
{

FRESULT create_dir(Sdcard &sd, DIR *dir, const char *dir_name) {

	FRESULT res;

	// Open the temp directory
	res = f_opendir(dir, dir_name);

	// If it doesn't exist, create it
	if (res == FR_NO_PATH)
		res = f_mkdir(dir_name);

	// If we got an error opening or creating a dir
	// try reloading the SDCard, then opening the dir (and creating if needed)
	if (res == FR_OK)
		return FR_OK;

	if (!sd.reload_disk())
		return FR_DISK_ERR;

	res = f_opendir(dir, dir_name);
	if (res == FR_NO_PATH)
		res = f_mkdir(dir_name);
	return res;
}

FRESULT reload_sample_file(FIL *fil, Sample *s_sample, Sdcard &sd) {
	FRESULT res;

	// Try closing and re-opening file
	f_close(fil);
	res = f_open(fil, s_sample->filename, FA_READ);

	// If it fails, try re-mounting the sd card and opening again
	if (res != FR_OK) {
		f_close(fil);

		res = sd.reload_disk() ? FR_OK : FR_INT_ERR;
		if (res == FR_OK)
			res = f_open(fil, s_sample->filename, FA_READ);

		if (res != FR_OK)
			f_close(fil);
	}

	return res;
}

//
// Load the sample header from the provided file
//
uint32_t load_sample_header(Sample *s_sample, FIL *sample_file) {
	WaveHeader sample_header;
	WaveFmtChunk fmt_chunk;
	FRESULT res;
	UINT br;
	uint32_t rd;
	WaveChunk chunk;
	uint32_t next_chunk_start;

	rd = sizeof(WaveHeader);
	res = f_read(sample_file, &sample_header, rd, &br);

	// TODO: return this error?
	uint32_t g_error = 0;

	if (res != FR_OK) {
		g_error |= HEADER_READ_FAIL;
		return g_error;
	} // file not opened
	else if (br < rd)
	{
		g_error |= FILE_WAVEFORMATERR;
		f_close(sample_file);
		return g_error;
	} // file ended unexpectedly when reading first header
	else if (!is_valid_wav_header(sample_header))
	{
		g_error |= FILE_WAVEFORMATERR;
		f_close(sample_file);
		return g_error;
	} // first header error (not a valid wav file)

	// Look for a WaveFmtChunk (which starts off as a chunk)
	chunk.chunkId = 0;
	rd = sizeof(WaveChunk);

	while (chunk.chunkId != ccFMT) {
		res = f_read(sample_file, &chunk, rd, &br);

		if (res != FR_OK) {
			g_error |= HEADER_READ_FAIL;
			f_close(sample_file);
			return g_error;
		}
		if (br < rd) {
			g_error |= FILE_WAVEFORMATERR;
			f_close(sample_file);
			return g_error;
		}

		// Fix an odd-sized chunk, it should always be even
		if (chunk.chunkSize & 0b1)
			chunk.chunkSize++;

		next_chunk_start = f_tell(sample_file) + chunk.chunkSize;
		// fast-forward to the next chunk
		if (chunk.chunkId != ccFMT)
			f_lseek(sample_file, next_chunk_start);
	}

	// Go back to beginning of chunk --probably could do this more elegantly by removing fmtID and fmtSize from
	// WaveFmtChunk and just reading the next bit of data
	f_lseek(sample_file, f_tell(sample_file) - br);

	// Re-read the whole chunk (or at least the fields we need) since it's a WaveFmtChunk
	rd = sizeof(WaveFmtChunk);
	res = f_read(sample_file, &fmt_chunk, rd, &br);

	if (res != FR_OK) {
		g_error |= HEADER_READ_FAIL;
		return g_error;
	} // file not read
	else if (br < rd)
	{
		g_error |= FILE_WAVEFORMATERR;
		f_close(sample_file);
		return g_error;
	} // file ended unexpectedly when reading format header
	else if (!is_valid_format_chunk(fmt_chunk))
	{
		g_error |= FILE_WAVEFORMATERR;
		f_close(sample_file);
		return g_error;
	} // format header error (not a valid wav file)
	// We found the 'fmt ' chunk, now skip to the next chunk
	// Note: this is necessary in case the 'fmt ' chunk is not exactly sizeof(WaveFmtChunk) bytes, even
	// though that's how many we care about
	f_lseek(sample_file, next_chunk_start);

	// Look for the DATA chunk
	chunk.chunkId = 0;
	rd = sizeof(WaveChunk);

	while (chunk.chunkId != ccDATA) {
		res = f_read(sample_file, &chunk, rd, &br);

		if (res != FR_OK) {
			g_error |= HEADER_READ_FAIL;
			f_close(sample_file);
			return FR_INT_ERR;
		}
		if (br < rd) {
			g_error |= FILE_WAVEFORMATERR;
			f_close(sample_file);
			return FR_INT_ERR;
		}

		// Fix an odd-sized chunk, it should always be even
		if (chunk.chunkSize & 0b1)
			chunk.chunkSize++;

		// keeping scanning until we find the data chunk
		if (chunk.chunkId != ccDATA) {
			f_lseek(sample_file, f_tell(sample_file) + chunk.chunkSize);
			continue;
		}

		// check valid data chunk size
		if (chunk.chunkSize == 0) {
			f_close(sample_file);
			return FR_INT_ERR;
		}

		// Check the file is really as long as the data chunkSize says it is
		if (f_size(sample_file) < (f_tell(sample_file) + chunk.chunkSize)) {
			chunk.chunkSize = f_size(sample_file) - f_tell(sample_file);
		}

		s_sample->sampleSize = chunk.chunkSize;
		s_sample->sampleByteSize = fmt_chunk.bitsPerSample >> 3;
		s_sample->sampleRate = fmt_chunk.sampleRate;
		s_sample->numChannels = fmt_chunk.numChannels;
		s_sample->blockAlign = fmt_chunk.numChannels * fmt_chunk.bitsPerSample >> 3;
		s_sample->startOfData = f_tell(sample_file);

		if (fmt_chunk.audioFormat == 0xFFFE)
			s_sample->PCM = 3;
		else
			s_sample->PCM = fmt_chunk.audioFormat;

		s_sample->file_status = FileStatus::Found;
		s_sample->inst_end = s_sample->sampleSize;
		s_sample->inst_size = s_sample->sampleSize;
		s_sample->inst_start = 0;
		s_sample->inst_gain = 1.0;

		// TODO: populate from cues chunk
		s_sample->num_cues = 0;
		break;
	}

	// TODO: populate cues from labels txt file
	// if s_sample->num_cues == 0
	return 0;
}

//
// new_filename()
//
// Creates a new filename in the proper format
// The proper format is:
// PREFIX + TAKE# + SUFFIX + EXTENSION
// For the STS we set prefix to null and put the sample slot# after the suffix, so it's just:
// 		001-Sample02.wav
// Where 02 is the slot# we recorded into, and 001 is the Take#
// We also determine the best directory to place this file in, and make that part of path:
// 		BankFolder/001-Sample02.wav --> path
//
#define SLOT_DIGITS 2 /* number of digits in the Slot number: since we have max 10 slots, we need 2 digits */
#define TAKE_DIGITS 3 /* number of digits in the Take number: 3 means "001", 4 means "0001", etc. */
#define WAV_EXT ".wav"
FRESULT new_filename(uint8_t bank_idx, uint8_t sample_num, char *path, Sdcard &sd, Bank &bank) {
	uint8_t i;
	FRESULT res;
	DIR dir;
	uint32_t sz;

	char slot_str[SLOT_DIGITS + 1];
	char take_str[TAKE_DIGITS + 1];
	char slot_prefix[10];
	char slot_suffix[10];
	char timestamp_str[12];
	uint16_t highest_num, num;
	char filename[FF_MAX_LFN + 1];
	uint8_t take_pos;
	uint32_t max_take_num;
	uint8_t do_add_timestamp = 0;

	//
	// Figure out the folder to put a new recording in:
	// 1st choice: Same folder as the sample already existing in the current REC bank/sample
	// 2nd choice: Same folder as the first existing sample in the current REC bank
	// 3rd choice: default folder name for current REC bank
	//

	path[0] = 0;

	// 1st) Check for the current REC sample/bank folder:
	// If file name is not blank, split the filename at the last '/'
	if (bank[sample_num].filename[0]) {
		if (str_rstr(bank[sample_num].filename, '/', path) != 0)
			path[str_len(path) - 1] = 0; // truncate the last '/'
		else
			path[0] = 0;
	}
	// 2nd) Check all the samples in REC bank
	if (!path[0]) {
		for (i = 0; i < NumSamplesPerBank; i++)
			if (bank[i].filename[0]) {
				if (str_rstr(bank[i].filename, '/', path) != 0)
					path[str_len(path) - 1] = 0; // truncate the last '/'
				else
					path[0] = 0;
			}
	}
	// 3rd) Use the default bank name if the above methods failed
	if (!path[0])
		bank_to_color(bank_idx, path);

	res = create_dir(sd, &dir, path);
	if (res == FR_OK)
		res = f_opendir(&dir, path);

	if (res != FR_OK)
		return FR_INT_ERR; // fail

	//
	// Create the file name
	//
	//
	// Find the highest numbered file (of our proper format), and name this file one higher
	// If no files are found of our format, start at 1

	highest_num = 0;

	// Compute the slot prefix
	//  intToStr(sample_num+1, slot_str, 2);
	//  str_cat(slot_prefix, "Slot", slot_str);
	//  str_cat(slot_prefix, slot_prefix, "-");
	slot_prefix[0] = '\0';

	// Compute the slot suffix
	str_cpy(slot_suffix, "-Sample");

	// Compute the slot string
	intToStr(sample_num + 1, slot_str, SLOT_DIGITS);

	while (1) {
		uint8_t findres = sd.find_next_ext_in_dir(&dir, WAV_EXT, filename);

		if (findres != FR_OK && findres != 0xFF) {
			f_closedir(&dir);

			// filesystem error reading directory,
			// Just use a timestamp to be safe
			sz = str_len(path);
			path[sz++] = '/';
			sz += intToStr(sample_num, &(path[sz]), 2);
			path[sz++] = '-';
			sz += intToStr(HAL_GetTick(), &(path[sz]), 0);
			path[sz++] = '.';
			path[sz++] = 'w';
			path[sz++] = 'a';
			path[sz++] = 'v';
			path[sz++] = 0;

			return res;
		}
		if (findres == 0xFF) {
			// no more .wav files found
			// exit the while loop
			f_closedir(&dir);
			break;
		}
		if (filename[0]) {
			// See if the file is of the form we're looking for:

			// Must be exactly the right length
			if (str_len(filename) !=
				(str_len(slot_prefix) + TAKE_DIGITS + str_len(slot_suffix) + SLOT_DIGITS + str_len(WAV_EXT)))
				continue;

			// Must start with slot prefix
			if (!str_startswith_nocase(filename, slot_prefix))
				continue;

			// Must contain the slot suffix
			if (str_found(filename, slot_suffix) == 0)
				continue;

			// Three characters of the Take# must be digits
			take_pos = str_len(slot_prefix);
			if (!(filename[take_pos] >= '0' && filename[take_pos] <= '9' && filename[take_pos + 1] >= '0' &&
				  filename[take_pos + 1] <= '9' && filename[take_pos + 2] >= '0' && filename[take_pos + 2] <= '9'))
				continue;

			// Extract the take number string and convert to an int
			take_str[0] = filename[take_pos];
			take_str[1] = filename[take_pos + 1];
			take_str[2] = filename[take_pos + 2];
			take_str[3] = '\0';
			num = str_xt_int(take_str);

			// Update highest_num if we find a higher one
			if (num > highest_num)
				highest_num = num;
		}
	}

	// Go one higher than the highest found
	highest_num++;

	// Boundary check highest_num is not greater than the max number of digits
	max_take_num = 1;
	for (i = 0; i < TAKE_DIGITS; i++)
		max_take_num *= 10;
	if (highest_num >= max_take_num) {
		do_add_timestamp = 1;
		highest_num = (max_take_num - 1);
	} // stay at the max num if we surpass it, and later we'll add a timestamp

	// Create the filename with path
	str_cat(path, path, "/");
	str_cat(path, path, slot_prefix);
	intToStr(highest_num, take_str, 3);
	str_cat(path, path, take_str);

	if (do_add_timestamp) {
		str_cat(path, path, "-");
		intToStr(HAL_GetTick(), timestamp_str, 10);
		str_cat(path, path, timestamp_str);
	}
	str_cat(path, path, slot_suffix);
	str_cat(path, path, slot_str);
	str_cat(path, path, WAV_EXT);

	return FR_OK;
}

} // namespace SamplerKit
