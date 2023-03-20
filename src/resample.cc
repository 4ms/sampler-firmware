/*
 * resample.c - Hermetian resampling algorithm, optimized for speed (not code size or readability!)
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

#include "resample.hh"
#include "circular_buffer.hh"
#include <algorithm>

namespace SamplerKit
{

void wait_memory_ready() {
	// Memory::wait_until_ready();
}

static inline void inc_addr(CircularBuffer *buf, uint8_t blockAlign, bool rev) {
	if (!rev) {
		buf->out += blockAlign;

		if (buf->out >=
			buf->max) // This will not work if buf->max==0xFFFFFFFF, but luckily this is never the case with the STS!
		{
			buf->wrapping = 0;
			buf->out -= buf->size;
		}
	} else {
		if ((buf->out - buf->min) < blockAlign) {
			buf->out += buf->size - blockAlign;
			buf->wrapping = 1;
		} else
			buf->out -= blockAlign;
	}
}

static inline int32_t get_16b_sample_avg(uint32_t addr) {

	wait_memory_ready();

	uint32_t rd = (*((uint32_t *)addr));

	int16_t a = (int16_t)(rd >> 16);
	int16_t b = (int16_t)(rd & 0x0000FFFF);
	int32_t t = a + b;
	return t * 128; //*256 and averaged is /2
}

static inline int32_t get_16b_sample_right(uint32_t addr) {
	wait_memory_ready();
	int16_t r = *((int16_t *)(addr + 2));
	return ((int32_t)r) * 256;
}

static inline int32_t get_16b_sample_left(uint32_t addr) {
	wait_memory_ready();
	int16_t r = *((int16_t *)addr);
	return ((int32_t)r) * 256;
}

// FIXME:
// Before a call to this, it should take Flag::PlayBuffDiscontinuity and put the result in flush
// After a call to this, the caller must check if rs==1.f, and if so then it must set Flags::PlayBuffDiscontinuity
// FIXME:
// ch is fixed? in templated version we'll get different xm1, x0, etc for each <Chan>, so we can remove the array
// FIXME: use a span (out.data, buff_len)
void resample_read16_avg(
	float rs, CircularBuffer *buf, uint32_t buff_len, uint8_t block_align, int32_t *out, bool rev, bool flush) {
	static float fractional_pos[4] = {0, 0, 0, 0};
	static float xm1[4], x0[4], x1[4], x2[4];
	float a, b, c;
	uint32_t outpos;
	uint8_t ch;

	ch = 0;

	if (rs == 1.f) {
		for (outpos = 0; outpos < buff_len; outpos++) {
			inc_addr(buf, block_align, rev);
			out[outpos] = get_16b_sample_avg(buf->out);
		}
		// flags[PlayBuff1_Discontinuity + chan] = 1;

	} else {

		// fill the resampling buffer with three points
		if (flush) {
			// flags[PlayBuff1_Discontinuity + chan] = 0;

			inc_addr(buf, block_align, rev);
			x0[ch] = get_16b_sample_avg(buf->out);

			inc_addr(buf, block_align, rev);
			x1[ch] = get_16b_sample_avg(buf->out);

			inc_addr(buf, block_align, rev);
			x2[ch] = get_16b_sample_avg(buf->out);

			fractional_pos[ch] = 0.f;
		}

		outpos = 0;
		while (outpos < buff_len) {
			// Optimize for resample rates >= 4
			if (fractional_pos[ch] >= 4.f) {
				fractional_pos[ch] = fractional_pos[ch] - 4.f;

				// shift samples back one
				// and read a new sample
				inc_addr(buf, block_align, rev);
				xm1[ch] = get_16b_sample_avg(buf->out);

				inc_addr(buf, block_align, rev);
				x0[ch] = get_16b_sample_avg(buf->out);

				inc_addr(buf, block_align, rev);
				x1[ch] = get_16b_sample_avg(buf->out);

				inc_addr(buf, block_align, rev);
				x2[ch] = get_16b_sample_avg(buf->out);
			}
			// Optimize for resample rates >= 3
			if (fractional_pos[ch] >= 3.f) {
				fractional_pos[ch] = fractional_pos[ch] - 3.f;

				// shift samples back one
				// and read a new sample
				xm1[ch] = x2[ch];

				inc_addr(buf, block_align, rev);
				x0[ch] = get_16b_sample_avg(buf->out);

				inc_addr(buf, block_align, rev);
				x1[ch] = get_16b_sample_avg(buf->out);

				inc_addr(buf, block_align, rev);
				x2[ch] = get_16b_sample_avg(buf->out);
			}
			// Optimize for resample rates >= 2
			if (fractional_pos[ch] >= 2.f) {
				fractional_pos[ch] = fractional_pos[ch] - 2.f;

				// shift samples back one
				// and read a new sample
				xm1[ch] = x1[ch];
				x0[ch] = x2[ch];

				inc_addr(buf, block_align, rev);
				x1[ch] = get_16b_sample_avg(buf->out);

				inc_addr(buf, block_align, rev);
				x2[ch] = get_16b_sample_avg(buf->out);
			}
			// Optimize for resample rates >= 1
			if (fractional_pos[ch] >= 1.f) {
				fractional_pos[ch] = fractional_pos[ch] - 1.f;

				// shift samples back one
				// and read a new sample
				xm1[ch] = x0[ch];
				x0[ch] = x1[ch];
				x1[ch] = x2[ch];

				inc_addr(buf, block_align, rev);
				x2[ch] = get_16b_sample_avg(buf->out);
			}

			// calculate coefficients
			a = (3 * (x0[ch] - x1[ch]) - xm1[ch] + x2[ch]) / 2;
			b = 2 * x1[ch] + xm1[ch] - (5 * x0[ch] + x2[ch]) / 2;
			c = (x1[ch] - xm1[ch]) / 2;

			// calculate as many fractionally placed output points as we need
			while (fractional_pos[ch] < 1.f && outpos < buff_len) {
				float t_out = (((a * fractional_pos[ch]) + b) * fractional_pos[ch] + c) * fractional_pos[ch] + x0[ch];
				out[outpos++] = (int32_t)(std::clamp(t_out, -32768.f * 256.f, 32767.f * 256.f));
				// if (t_out >= 32767.f*256)
				// 	out[outpos++] = 32767*256;
				// else if (t_out <= -32767.f*256)
				// 	out[outpos++] = -32767*256;
				// else
				// 	out[outpos++] = t_out*256;

				fractional_pos[ch] += rs;
			}
		}
	}
}

void resample_read16_right(
	float rs, CircularBuffer *buf, uint32_t buff_len, uint8_t block_align, int32_t *out, bool rev, bool flush) {
	static float fractional_pos[4] = {0, 0, 0, 0};
	static float xm1[4], x0[4], x1[4], x2[4];
	float a, b, c;
	uint32_t outpos;
	float t_out;
	uint8_t ch;

	ch = 1;

	if (rs == 1.f) {
		for (outpos = 0; outpos < buff_len; outpos++) {
			inc_addr(buf, block_align, rev);
			out[outpos] = get_16b_sample_right(buf->out);
		}
		// flags[PlayBuff1_Discontinuity + chan] = 1;

	} else {

		// fill the resampling buffer with three points
		if (flush) {
			// flags[PlayBuff1_Discontinuity + chan] = 0;

			inc_addr(buf, block_align, rev);
			x0[ch] = get_16b_sample_right(buf->out);

			inc_addr(buf, block_align, rev);
			x1[ch] = get_16b_sample_right(buf->out);

			inc_addr(buf, block_align, rev);
			x2[ch] = get_16b_sample_right(buf->out);

			fractional_pos[ch] = 0.f;
		}

		outpos = 0;
		while (outpos < buff_len) {
			// Optimize for resample rates >= 4
			if (fractional_pos[ch] >= 4.f) {
				fractional_pos[ch] = fractional_pos[ch] - 4.f;

				// shift samples back one
				// and read a new sample
				inc_addr(buf, block_align, rev);
				xm1[ch] = get_16b_sample_right(buf->out);

				inc_addr(buf, block_align, rev);
				x0[ch] = get_16b_sample_right(buf->out);

				inc_addr(buf, block_align, rev);
				x1[ch] = get_16b_sample_right(buf->out);

				inc_addr(buf, block_align, rev);
				x2[ch] = get_16b_sample_right(buf->out);
			}
			// Optimize for resample rates >= 3
			if (fractional_pos[ch] >= 3.f) {
				fractional_pos[ch] = fractional_pos[ch] - 3.f;

				// shift samples back one
				// and read a new sample
				xm1[ch] = x2[ch];

				inc_addr(buf, block_align, rev);
				x0[ch] = get_16b_sample_right(buf->out);

				inc_addr(buf, block_align, rev);
				x1[ch] = get_16b_sample_right(buf->out);

				inc_addr(buf, block_align, rev);
				x2[ch] = get_16b_sample_right(buf->out);
			}
			// Optimize for resample rates >= 2
			if (fractional_pos[ch] >= 2.f) {
				fractional_pos[ch] = fractional_pos[ch] - 2.f;

				// shift samples back one
				// and read a new sample
				xm1[ch] = x1[ch];
				x0[ch] = x2[ch];

				inc_addr(buf, block_align, rev);
				x1[ch] = get_16b_sample_right(buf->out);

				inc_addr(buf, block_align, rev);
				x2[ch] = get_16b_sample_right(buf->out);
			}
			// Optimize for resample rates >= 1
			if (fractional_pos[ch] >= 1.f) {
				fractional_pos[ch] = fractional_pos[ch] - 1.f;

				// shift samples back one
				// and read a new sample
				xm1[ch] = x0[ch];
				x0[ch] = x1[ch];
				x1[ch] = x2[ch];

				inc_addr(buf, block_align, rev);
				x2[ch] = get_16b_sample_right(buf->out);
			}

			// calculate coefficients
			a = (3 * (x0[ch] - x1[ch]) - xm1[ch] + x2[ch]) / 2;
			b = 2 * x1[ch] + xm1[ch] - (5 * x0[ch] + x2[ch]) / 2;
			c = (x1[ch] - xm1[ch]) / 2;

			// calculate as many fractionally placed output points as we need
			while (fractional_pos[ch] < 1.f && outpos < buff_len) {
				t_out = (((a * fractional_pos[ch]) + b) * fractional_pos[ch] + c) * fractional_pos[ch] + x0[ch];
				out[outpos++] = (int32_t)(std::clamp(t_out, -32768.f * 256.f, 32767.f * 256.f));
				// if (t_out >= 32767.f)
				// 	out[outpos++] = 32767;
				// else if (t_out <= -32767.f)
				// 	out[outpos++] = -32767;
				// else
				// 	out[outpos++] = t_out;

				fractional_pos[ch] += rs;
			}
		}
	}
}

void resample_read16_left(
	float rs, CircularBuffer *buf, uint32_t buff_len, uint8_t block_align, int32_t *out, bool rev, bool flush) {
	static float fractional_pos[4] = {0, 0, 0, 0};
	static float xm1[4], x0[4], x1[4], x2[4];
	float a, b, c;
	uint32_t outpos;
	float t_out;
	uint8_t ch;

	ch = 0;

	if (rs == 1.f) {
		for (outpos = 0; outpos < buff_len; outpos++) {
			inc_addr(buf, block_align, rev);
			out[outpos] = get_16b_sample_left(buf->out);
		}
		// flags[PlayBuff1_Discontinuity + chan] = 1;

	} else {

		// fill the resampling buffer with three points
		if (flush) {
			// flags[PlayBuff1_Discontinuity + chan] = 0;

			inc_addr(buf, block_align, rev);
			x0[ch] = get_16b_sample_left(buf->out);

			inc_addr(buf, block_align, rev);
			x1[ch] = get_16b_sample_left(buf->out);

			inc_addr(buf, block_align, rev);
			x2[ch] = get_16b_sample_left(buf->out);

			fractional_pos[ch] = 0.f;
		}

		outpos = 0;
		while (outpos < buff_len) {
			// Optimize for resample rates >= 4
			if (fractional_pos[ch] >= 4.f) {
				fractional_pos[ch] = fractional_pos[ch] - 4.f;

				// shift samples back one
				// and read a new sample
				inc_addr(buf, block_align, rev);
				xm1[ch] = get_16b_sample_left(buf->out);

				inc_addr(buf, block_align, rev);
				x0[ch] = get_16b_sample_left(buf->out);

				inc_addr(buf, block_align, rev);
				x1[ch] = get_16b_sample_left(buf->out);

				inc_addr(buf, block_align, rev);
				x2[ch] = get_16b_sample_left(buf->out);
			}
			// Optimize for resample rates >= 3
			if (fractional_pos[ch] >= 3.f) {
				fractional_pos[ch] = fractional_pos[ch] - 3.f;

				// shift samples back one
				// and read a new sample
				xm1[ch] = x2[ch];

				inc_addr(buf, block_align, rev);
				x0[ch] = get_16b_sample_left(buf->out);

				inc_addr(buf, block_align, rev);
				x1[ch] = get_16b_sample_left(buf->out);

				inc_addr(buf, block_align, rev);
				x2[ch] = get_16b_sample_left(buf->out);
			}
			// Optimize for resample rates >= 2
			if (fractional_pos[ch] >= 2.f) {
				fractional_pos[ch] = fractional_pos[ch] - 2.f;

				// shift samples back one
				// and read a new sample
				xm1[ch] = x1[ch];
				x0[ch] = x2[ch];

				inc_addr(buf, block_align, rev);
				x1[ch] = get_16b_sample_left(buf->out);

				inc_addr(buf, block_align, rev);
				x2[ch] = get_16b_sample_left(buf->out);
			}
			// Optimize for resample rates >= 1
			if (fractional_pos[ch] >= 1.f) {
				fractional_pos[ch] = fractional_pos[ch] - 1.f;

				// shift samples back one
				// and read a new sample
				xm1[ch] = x0[ch];
				x0[ch] = x1[ch];
				x1[ch] = x2[ch];

				inc_addr(buf, block_align, rev);
				x2[ch] = get_16b_sample_left(buf->out);
			}

			// calculate coefficients
			a = (3 * (x0[ch] - x1[ch]) - xm1[ch] + x2[ch]) / 2;
			b = 2 * x1[ch] + xm1[ch] - (5 * x0[ch] + x2[ch]) / 2;
			c = (x1[ch] - xm1[ch]) / 2;

			// calculate as many fractionally placed output points as we need
			while (fractional_pos[ch] < 1.f && outpos < buff_len) {
				t_out = (((a * fractional_pos[ch]) + b) * fractional_pos[ch] + c) * fractional_pos[ch] + x0[ch];
				out[outpos++] = (int32_t)(std::clamp(t_out, -32768.f * 256.f, 32767.f * 256.f));
				// if (t_out >= 32767.f)
				// 	out[outpos++] = 32767;
				// else if (t_out <= -32767.f)
				// 	out[outpos++] = -32767;
				// else
				// 	out[outpos++] = t_out;

				fractional_pos[ch] += rs;
			}
		}
	}
}

} // namespace SamplerKit
