#pragma once
#include "conf/sdram_conf.hh"
#include "drivers/adc_builtin_conf.hh"
#include "drivers/dma_config_struct.hh"
#include "drivers/pin.hh"
#include "drivers/pin_change_conf.hh"
#include "drivers/timekeeper.hh"
#include "drivers/uart.hh"

namespace Brain
{
constexpr inline int16_t MinPotChange = 10;
constexpr inline int16_t MinCVChange = 10;

// ADC Peripheral Conf:
struct CVAdcConf : mdrivlib::DefaultAdcPeriphConf {
	static constexpr auto resolution = mdrivlib::AdcResolution::Bits12;
	static constexpr auto adc_periph_num = mdrivlib::AdcPeriphNum::_1;
	static constexpr auto oversample = true;
	static constexpr auto oversampling_ratio = 1024;
	static constexpr auto oversampling_right_bitshift = mdrivlib::AdcOversampleRightBitShift::Shift10Right;
	static constexpr auto clock_div = mdrivlib::PLL_Div1;

	static constexpr bool enable_end_of_sequence_isr = true;
	static constexpr bool enable_end_of_conversion_isr = false;

	struct DmaConf : mdrivlib::DefaultAdcPeriphConf::DmaConf {
		static constexpr auto DMAx = 2;
		static constexpr auto StreamNum = 7;
		static constexpr auto RequestNum = DMA_REQUEST_ADC1;
		static constexpr auto dma_priority = Low;
		static constexpr IRQn_Type IRQn = DMA2_Stream7_IRQn;
		static constexpr uint32_t pri = 0;
		static constexpr uint32_t subpri = 0;
	};
};

struct PotAdcConf : mdrivlib::DefaultAdcPeriphConf {
	static constexpr auto resolution = mdrivlib::AdcResolution::Bits12;
	static constexpr auto adc_periph_num = mdrivlib::AdcPeriphNum::_2;
	static constexpr auto oversample = true;
	static constexpr auto oversampling_ratio = 1024;
	static constexpr auto oversampling_right_bitshift = mdrivlib::AdcOversampleRightBitShift::Shift10Right;
	static constexpr auto clock_div = mdrivlib::PLL_Div1;

	static constexpr bool enable_end_of_sequence_isr = true;
	static constexpr bool enable_end_of_conversion_isr = false;

	struct DmaConf : mdrivlib::DefaultAdcPeriphConf::DmaConf {
		static constexpr auto DMAx = 2;
		static constexpr auto StreamNum = 6;
		static constexpr auto RequestNum = DMA_REQUEST_ADC2;
		static constexpr auto dma_priority = Low;
		static constexpr IRQn_Type IRQn = DMA2_Stream6_IRQn;
		static constexpr uint32_t pri = 0;
		static constexpr uint32_t subpri = 0;
	};
};

// memory_conf:
constexpr inline uint32_t MemoryStartAddr = 0xD0000000;
constexpr inline uint32_t MemorySizeBytes = 0x08000000; // TODO: check this value
constexpr inline uint32_t MemoryEndAddr = MemoryStartAddr + MemorySizeBytes;

using RAMSampleT = int16_t;
constexpr inline uint32_t MemorySampleSize = sizeof(RAMSampleT);
constexpr inline uint32_t MemorySamplesNum = MemorySizeBytes / MemorySampleSize;

// clock sync conf
struct LRClkPinChangeConf : mdrivlib::DefaultPinChangeConf {
	static constexpr uint32_t pin = 12;
	static constexpr mdrivlib::GPIO port = mdrivlib::GPIO::D;
	static constexpr bool on_rising_edge = true;
	static constexpr bool on_falling_edge = false;
	static constexpr uint32_t priority1 = 0;
	static constexpr uint32_t priority2 = 0;
};

const mdrivlib::TimekeeperConfig param_update_task_conf = {
	.TIMx = TIM6,
	.period_ns = 1'000'000'000 / 6000, // 6kHz
	.priority1 = 2,
	.priority2 = 3,
};

namespace Debug
{
struct Disabled {
	static void high() {}
	static void low() {}
};

using Pin0 = Disabled;
using Pin1 = Disabled;
// using Pin0 = mdrivlib::FPin<mdrivlib::GPIO::C, mdrivlib::PinNum::_6>;
// using Pin1 = mdrivlib::FPin<mdrivlib::GPIO::C, mdrivlib::PinNum::_7>;

using Pin2 = mdrivlib::FPin<mdrivlib::GPIO::G, mdrivlib::PinNum::_12>;
using Pin3 = mdrivlib::FPin<mdrivlib::GPIO::E, mdrivlib::PinNum::_11>;
}; // namespace Debug

} // namespace Brain
