#pragma once
#include "conf/flash_layout.hh"
#include "conf/rcc_conf.hh"
#include "conf/sdram_conf.hh"
#include "console.hh"
#include "debug.hh"
#include "drivers/sdram.hh"
#include "drivers/system.hh"
#include "drivers/system_clocks.hh"

namespace SamplerKit
{

struct System {
	System() {
		mdrivlib::System::SetVectorTable(AppStartAddr);
		mdrivlib::SystemClocks::init_clocks(osc_conf, clk_conf, rcc_periph_conf);

		// TODO: F7 only:
		//  mdrivlib::SDRAMPeriph sdram{Brain::SDRAM_conf, Brain::SdramBank, Brain::SdramKernelClock};
		//  SCB_InvalidateICache();
		//  SCB_EnableICache();

		Debug::Pin0{};
		Debug::Pin1{};
		Debug::Pin2{};
		Debug::Pin3{};
		Console::init();
	}
};

} // namespace SamplerKit
