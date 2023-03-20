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
#include "errors.hh"
#include "ff.h"
#include "sdcard.hh"
#include "str_util.h"
#include "wavefmt.hh"

namespace SamplerKit
{

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

	return (res);
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

	else
	{
		// Look for a WaveFmtChunk (which starts off as a chunk)
		chunk.chunkId = 0;
		rd = sizeof(WaveChunk);

		while (chunk.chunkId != ccFMT) {
			res = f_read(sample_file, &chunk, rd, &br);

			if (res != FR_OK) {
				g_error |= HEADER_READ_FAIL;
				f_close(sample_file);
				break;
			}
			if (br < rd) {
				g_error |= FILE_WAVEFORMATERR;
				f_close(sample_file);
				break;
			}

			// Fix an odd-sized chunk, it should always be even
			if (chunk.chunkSize & 0b1)
				chunk.chunkSize++;

			next_chunk_start = f_tell(sample_file) + chunk.chunkSize;
			// fast-forward to the next chunk
			if (chunk.chunkId != ccFMT)
				f_lseek(sample_file, next_chunk_start);
		}

		if (chunk.chunkId == ccFMT) {
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
			else
			{
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
						break;
					}
					if (br < rd) {
						g_error |= FILE_WAVEFORMATERR;
						f_close(sample_file);
						break;
					}

					// Fix an odd-sized chunk, it should always be even
					if (chunk.chunkSize & 0b1)
						chunk.chunkSize++;

					// fast-forward to the next chunk
					if (chunk.chunkId != ccDATA)
						f_lseek(sample_file, f_tell(sample_file) + chunk.chunkSize);

					// Set the sampleSize as defined in the chunk
					else {
						if (chunk.chunkSize == 0) {
							f_close(sample_file);
							break;
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

						s_sample->file_found = 1;
						s_sample->inst_end = s_sample->sampleSize;
						s_sample->inst_size = s_sample->sampleSize;
						s_sample->inst_start = 0;
						s_sample->inst_gain = 1.0;

						return g_error;

					} // else chunk
				}	  // while chunk
			}		  // is_valid_format_chunk
		}			  // if ccFMT
	}				  // no file error

	return g_error;
}

} // namespace SamplerKit