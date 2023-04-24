#include "adc.h"
#include "adc_pins.h"
#include "analog_conditioning.h"
#include "dig_inout_pins.hh"
#include "drivers/pin.hh"
#include "drivers/timekeeper.hh"
#include <functional>

namespace
{
mdrivlib::Timekeeper read_task;
uint16_t buf;
} // namespace

void init_gate_in() {
	DigIO::TrigJack init;
	DigIO::EOJack debug_out_init;

	// builtinAdcSetup adc_setup = {
	// 	.gpio = CV_SHAPE_GPIO_Port,
	// 	.pin = CV_SHAPE_Pin,
	// 	.channel = CV_SHAPE_Channel,
	// 	.sample_time = ADC_SAMPLETIME_56CYCLES,
	// };
	// ADC_Init(ADC1, &buf, 1, &adc_setup, 1);
}

bool gate_in_read(uint32_t) {
	bool j = DigIO::TrigJack::read();
	if (j)
		DigIO::EOJack::high();
	else
		DigIO::EOJack::low();
	return j;
}

void start_reception(uint32_t sample_rate, std::function<void()> &&callback) {
	const mdrivlib::TimekeeperConfig conf{
		.TIMx = TIM7,
		.period_ns = 1'000'000'000 / sample_rate,
		.priority1 = 1,
		.priority2 = 1,
	};
	read_task.init(conf, std::move(callback));
	read_task.start();
}
