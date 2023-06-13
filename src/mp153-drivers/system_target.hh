#pragma once
#include "conf/sdram_conf.hh"
#include "drivers/stm32xx.h"

namespace SamplerKit
{
struct SystemTarget {
	static void init() {}
};
} // namespace SamplerKit

inline void recover_from_task_fault() {
	//???
}
