#pragma once
#include <cstdint>

namespace SamplerKit
{

namespace TimingCalcs
{
inline uint32_t calc_trig_delay(uint8_t trig_delay_setting) {
	if (trig_delay_setting > 20 || trig_delay_setting < 1)
		trig_delay_setting = 8;
	return trig_delay_setting;
	// if (trig_delay_setting <= 8)
	// 	return (trig_delay_setting * 12) - 8;
	// if (trig_delay_setting == 9)
	// 	return 180;
	// if (trig_delay_setting == 10)
	// 	return 360;
	// if (trig_delay_setting > 10)
	// 	return (trig_delay_setting * 72);
	// return (0);
}

inline uint32_t calc_pitch_latch_time(uint8_t trig_delay_setting) {
	if (trig_delay_setting > 10 || trig_delay_setting < 1)
		trig_delay_setting = 1;

	return trig_delay_setting;
	// if (trig_delay_setting <= 8)
	// 	return ((trig_delay_setting - 1) * 6);
	// if (trig_delay_setting == 9)
	// 	return 50;
	// if (trig_delay_setting == 10)
	// 	return 60;
	// return (0);
}

inline float calc_fade_updown_rate(float sample_rate, float ht16_chan_buff_len, uint8_t time_ms) {
	// map 0..255 => 1/HT16_CHAN_BUFF_LEN .. 1/(0.05 * 44100.f) = one buffer to 50ms
	float range = 0.05f * sample_rate - ht16_chan_buff_len;
	float offset = ht16_chan_buff_len;
	return 1.0f / ((((float)time_ms) / 255.0f) * range + offset);
}

} // namespace TimingCalcs
} // namespace SamplerKit
