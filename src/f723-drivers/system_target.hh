#pragma once
#include "brain_conf.hh"
#include "drivers/sdram.hh"
#include "drivers/stm32xx.h"
#include "sdram_conf.hh"

namespace SamplerKit
{

struct SystemTarget {
	static void init() {
		HAL_Init();
		HAL_NVIC_SetPriorityGrouping(NVIC_PRIORITYGROUP_2);

		mdrivlib::SDRAMPeriph sdram{Brain::SDRAM_conf, Brain::SdramBank, Brain::SdramKernelClock};
		SCB_InvalidateICache();
		SCB_EnableICache();

		// SCB_EnableDCache();
		// SCB_CleanInvalidateDCache();

		// Data Cache not needed for this app:
		// Even with max modulation and resampling rate, audio processing loop never exceeds 25% load
		// Disabling DCache keeps the DMA transfers more simple
		// SCB_DisableDCache();
	}
};
} // namespace SamplerKit
