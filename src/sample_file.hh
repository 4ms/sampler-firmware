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

#pragma once
#include "ff.h"
#include "sdcard.hh"

namespace SamplerKit
{
enum class FileStatus : uint8_t { NotFound = 0, Found = 1, NewFile = 2 };

struct Sample {
	char filename[FF_MAX_LFN];
	uint32_t sampleSize;
	uint32_t startOfData;
	uint8_t sampleByteSize;
	uint32_t sampleRate;
	uint8_t numChannels;
	uint8_t blockAlign;

	uint32_t inst_start;
	uint32_t inst_end;
	uint32_t inst_size;
	float inst_gain;

	uint16_t PCM;
	FileStatus file_status;

	uint8_t num_cues;
	uint32_t cue[4];

	Sample() { clear(); }
	void clear() {
		filename[0] = 0;
		sampleSize = 0;
		sampleByteSize = 0;
		sampleRate = 0;
		numChannels = 0;
		blockAlign = 0;
		startOfData = 0;
		PCM = 0;
		file_status = FileStatus::NotFound;
		inst_start = 0;
		inst_end = 0;
		inst_size = 0;
		inst_gain = 1.0;
		num_cues = 0;
	}
};

using Bank = std::array<Sample, NumSamplesPerBank>;
using SampleList = std::array<Bank, MaxNumBanks>;

constexpr auto SampleSize = sizeof(Sample);
constexpr auto SampleListSize = sizeof(SampleList);

uint32_t load_sample_header(Sample *s_sample, FIL *sample_file);
FRESULT reload_sample_file(FIL *fil, Sample *s_sample, Sdcard &sd);
FRESULT create_dir(Sdcard &sd, DIR *dir, const char *dir_name);
FRESULT new_filename(uint8_t bank_idx, uint8_t sample_num, char *path, Sdcard &sd, Bank &bank);

} // namespace SamplerKit
