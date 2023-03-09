#pragma once
#include "conf/sd_conf.hh"
#include "drivers/sdcard.hh"

namespace SamplerKit
{
struct Sdcard {
	mdrivlib::SDCard<Brain::SDCardConf> sd;

	Sdcard() {
		//
	}
};
} // namespace SamplerKit
