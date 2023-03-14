#pragma once
#include "conf/sdram_conf.hh"
#include "drivers/stm32xx.h"

namespace SamplerKit
{

struct SystemTarget {
	static void init() {
		mdrivlib::SDRAMPeriph sdram{Brain::SDRAM_conf, Brain::SdramBank, Brain::SdramKernelClock};
		SCB_InvalidateICache();
		SCB_DisableICache();
	}
};
} // namespace SamplerKit
