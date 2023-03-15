#pragma once
#include "conf/sd_conf.hh"
#include "fatfs/fat_file_io.hh"
#include "fatfs/sdcard_ops.hh"

namespace SamplerKit
{
struct Sdcard {
	// mdrivlib::SDCard<Brain::SDCardConf> sd;
	SDCardOps<Brain::SDCardConf> sdcard_ops;
	FatFileIO sdcard{&sdcard_ops, Volume::SDCard};

	Sdcard() {
		//
	}

	bool reload_disk() {
		if (!sdcard.mount_disk()) {
			err_cant_mount = true;
			return false;
		}
		return true;
	}

	//
	// Create a fast-lookup table (linkmap)
	//
	DWORD chan_clmt[SamplerKit::NumSamplesPerBank][256];

	FRESULT create_linkmap(FIL *fil, uint8_t samplenum) {
		FRESULT res;

		fil->cltbl = chan_clmt[samplenum];
		chan_clmt[samplenum][0] = 256;

		res = f_lseek(fil, CREATE_LINKMAP);
		return (res);
	}

	bool err_cant_mount = false;
};
} // namespace SamplerKit
