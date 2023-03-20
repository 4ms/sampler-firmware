/*
 * circular_buffer.h - routines for handling circular buffers
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
#include <cstdint>

struct CircularBuffer {
	uint32_t in;
	uint32_t out;
	uint32_t min;
	uint32_t max;
	uint32_t size; // should always be max-min
	bool wrapping;

	CircularBuffer() { init(); }
	void init() {
		wrapping = 0;
		in = min;
		out = min;
	}

	void wait_memory_ready() {
		// Memory::wait_until_ready();
	}

	uint32_t memory_read16_cb(int16_t *rd_buff, uint32_t num_samples, uint8_t decrement) {
		uint32_t num_filled = 0;
		for (uint32_t i = 0; i < num_samples; i++) {

			if (out == in)
				num_filled = 1;
			else if (num_filled)
				num_filled++;

			if (num_filled)
				rd_buff[i] = 0;
			else {
				wait_memory_ready();
				rd_buff[i] = *((int16_t *)(out));
			}

			offset_out_address(2, decrement);
			out = (out & 0xFFFFFFFE);
		}
		return num_filled;
	}

	uint32_t memory_read24_cb(uint8_t *rd_buff, uint32_t num_samples, uint8_t decrement) {
		uint32_t i;
		uint32_t num_filled = 0;
		uint32_t num_bytes = num_samples * 3;

		for (i = 0; i < num_bytes; i++) {
			wait_memory_ready();
			rd_buff[i] = *((uint8_t *)(out));
			offset_out_address(1, decrement);
		}
		return num_filled;
	}

	// Grab 16-bit ints and write them into b as 16-bit ints
	// num_words should be the number of 32-bit words to read from wr_buff (bytes>>2)
	uint32_t memory_write_16as16(uint32_t *wr_buff, uint32_t num_words, uint8_t decrement) {
		uint32_t i;
		uint8_t heads_crossed = 0;
		uint8_t start_polarity, end_polarity, start_wrap, end_wrap;

		// detect head-crossing:
		start_polarity = (in < out) ? 0 : 1;
		start_wrap = wrapping;

		for (i = 0; i < num_words; i++) {
			wait_memory_ready();
			*((uint32_t *)in) = wr_buff[i];
			offset_in_address(4, decrement);
		}

		end_polarity = (in < out) ? 0 : 1;
		end_wrap = wrapping; // 0 or 1

		// start_polarity + end_polarity  is (0/2 if no change, 1 if change)
		// start_wrap + end_wrap is (0/2 if no change, 1 if change)
		// Thus the sum of all four is even unless just polarity or just wrap changes (but not both)

		if ((end_wrap + start_wrap + start_polarity + end_polarity) & 0b01) // if (sum is odd)
			return 1; // warning: in pointer and out pointer crossed
		else
			return 0; // pointers did not cross
	}

	// Grab 24-bit words and write them into b as 16-bit values
	uint32_t memory_write_24as16(uint8_t *wr_buff, uint32_t num_bytes, uint8_t decrement) {
		uint32_t i;
		uint8_t heads_crossed = 0;
		uint8_t start_polarity, end_polarity, start_wrap, end_wrap;

		start_polarity = (in < out) ? 0 : 1;
		start_wrap = wrapping;

		for (i = 0; i < num_bytes; i += 3) // must be a multiple of 3!
		{
			wait_memory_ready();
			*((int16_t *)in) = (int16_t)(wr_buff[i + 2] << 8 | wr_buff[i + 1]);
			offset_in_address(2, decrement);
		}

		end_polarity = (in < out) ? 0 : 1;
		end_wrap = wrapping; // 0 or 1

		if ((end_wrap + start_wrap + start_polarity + end_polarity) & 0b01)
			return 1; // warning: in pointer and out pointer crossed
		else
			return 0; // pointers did not cross
	}

	// Grab 32-bit words and write them into b as 16-bit values
	uint32_t memory_write_32ias16(uint8_t *wr_buff, uint32_t num_bytes, uint8_t decrement) {
		uint32_t i;
		uint8_t heads_crossed = 0;
		uint8_t start_polarity, end_polarity, start_wrap, end_wrap;

		start_polarity = (in < out) ? 0 : 1;
		start_wrap = wrapping;

		for (i = 0; i < num_bytes; i += 4) {
			wait_memory_ready();
			*((int16_t *)in) = (int16_t)(wr_buff[i + 3] << 8 | wr_buff[i + 2]);
			offset_in_address(2, decrement);
		}

		end_polarity = (in < out) ? 0 : 1;
		end_wrap = wrapping; // 0 or 1

		if ((end_wrap + start_wrap + start_polarity + end_polarity) & 0b01)
			return 1; // warning: in pointer and out pointer crossed
		else
			return 0; // pointers did not cross
	}

	// Grab 32-bit floats and write them into b as 16-bit values
	uint32_t memory_write_32fas16(float *wr_buff, uint32_t num_floats, uint8_t decrement) {
		uint32_t i;
		uint8_t heads_crossed = 0;
		uint8_t start_polarity, end_polarity, start_wrap, end_wrap;

		start_polarity = (in < out) ? 0 : 1;
		start_wrap = wrapping;

		for (i = 0; i < num_floats; i++) {
			wait_memory_ready();
			if (wr_buff[i] >= 1.f)
				*((int16_t *)in) = 32767;
			else if (wr_buff[i] <= -1.F)
				*((int16_t *)in) = -32768;
			else
				*((int16_t *)in) = (int16_t)(wr_buff[i] * 32767.f);

			offset_in_address(2, decrement);
		}

		end_polarity = (in < out) ? 0 : 1;
		end_wrap = wrapping; // 0 or 1

		if ((end_wrap + start_wrap + start_polarity + end_polarity) & 0b01)
			return 1; // warning: in pointer and out pointer crossed
		else
			return 0; // pointers did not cross
	}

	// Grab 8-bit ints from wr_buff and write them into b as 16-bit ints
	uint32_t memory_write_8as16(uint8_t *wr_buff, uint32_t num_bytes, uint8_t decrement) {
		uint32_t i;
		uint8_t heads_crossed = 0;
		uint8_t start_polarity, end_polarity, start_wrap, end_wrap;

		// setup to detect head-crossing:
		start_polarity = (in < out) ? 0 : 1;
		start_wrap = wrapping;

		for (i = 0; i < num_bytes; i++) {
			wait_memory_ready();
			*((int16_t *)in) = ((int16_t)(wr_buff[i]) - 128) * 256;
			offset_in_address(2, decrement);
		}

		end_polarity = (in < out) ? 0 : 1;
		end_wrap = wrapping; // 0 or 1

		if ((end_wrap + start_wrap + start_polarity + end_polarity) & 0b01) // if (sum is odd)
			return 1; // warning: in pointer and out pointer crossed
		else
			return 0; // pointers did not cross
	}

	uint32_t memory_write16_cb(int16_t *wr_buff, uint32_t num_samples, uint8_t decrement) {
		uint32_t i;
		uint32_t heads_crossed = 0;

		in = (in & 0xFFFFFFFE);

		for (i = 0; i < num_samples; i++) {
			wait_memory_ready();
			*((int16_t *)in) = wr_buff[i];
			offset_in_address(2, decrement);

			if (in == out) // don't consider the heads being crossed if they begin at the same place
				heads_crossed = out;
		}

		return heads_crossed;
	}

	uint8_t offset_in_address(uint32_t amt, bool subtract) {
		if (!subtract) {
			if ((max - in) <= amt) // same as "if ((in + amt) >= max)" but doing the math this way avoids
								   // overflow in case max == 0xFFFFFFFF
			{
				in -= size - amt;
				if (wrapping)
					return 1; // warning: already wrapped, and wrapped again!
				wrapping = 1;
			} else
				in += amt;
		} else {
			if ((in - min) < amt) // same as "if (in - amt) < min" but avoids using negative numbers in case amt > in
			{
				in += size - amt;
				if (!wrapping)
					return 1; // warning: already unwrapped!
				wrapping = 0;
			} else
				in -= amt;
		}
		return 0;
	}

	uint8_t offset_out_address(uint32_t amt, bool subtract) {
		if (!subtract) {
			if ((max - out) <= amt) // same as "if (out + amt) > max" but doing the math this way avoids
									// overflow in case max == 0xFFFFFFFF
			{
				out -= size - amt;
				if (!wrapping)
					return 1; // warning: already unwrapped!
				wrapping = 0;
			} else
				out += amt;
		} else {
			if ((out - min) < amt) // same as "if (out - amt) < min" but avoids using negative numbers in case amt > out
			{
				out += size - amt;
				if (wrapping)
					return 1; // warning: already wrapped, and wrapped again!
				wrapping = 1;
			} else
				out -= amt;
		}
		return 0;
	}

	static uint32_t distance_points(uint32_t leader, uint32_t follower, uint32_t size, bool reverse) {
		if (reverse) {
			if (follower >= leader)
				return follower - leader;
			else // wrapping
				return (follower + size) - leader;
		} else {
			if (leader >= follower)
				return leader - follower;
			else // wrapping
				return (leader + size) - follower;
		}
	}

	uint32_t distance(bool reverse) {
		if (reverse) {
			if (out >= in)
				return out - in;
			else // wrapping
				return (out + size) - in;
		} else {
			if (in >= out)
				return in - out;
			else // wrapping
				return (in + size) - out;
		}
	}
};