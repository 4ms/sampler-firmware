#pragma once
#include "conf/sd_conf.hh"
#include "disk_ops.hh"
#include "drivers/sdcard.hh"

template<mdrivlib::SDCardConfC ConfT>
class SDCardOps : public DiskOps {
	constexpr static uint32_t SDBlockSize = 512;

public:
	mdrivlib::SDCard<ConfT> sd;

	enum class Status { NotInit, NoCard, Mounted };

	SDCardOps(SDCardOps &other) = delete;
	SDCardOps() = default;

	DSTATUS status() override { return (_status == Status::NotInit) ? (STA_NOINIT | STA_NODISK) : 0; }

	// FatFS calls this :
	// - in f_mkfs(),
	// - and when it mouts the disk in f_mount(_,_,1)
	// - and the first time FatFS attempts a read/write/stat if the disk is not yet mounted
	DSTATUS initialize() override {
		if (_status == Status::NotInit)
			sd.init();

		if (sd.detect_card()) {
			_status = Status::Mounted;
			// DEBUG: Test RX throughput
			// static bool tested = false;
			// if (!tested) {
			// 	tested = true;
			// 	uint32_t ms_per_10MB = sd.test_rx_speed(0, 10 * 2048);
			// 	float s_per_10MB = (float)ms_per_10MB / 1000.f;
			// 	float s_per_MB = s_per_10MB / 10.f;
			// 	printf_("%d ms/10MB = %fMB/s\n", ms_per_10MB, 1.f / s_per_MB);
			// }
			return 0;
		} else {
			_status = Status::NoCard;
			return STA_NODISK;
		}
	}

	DRESULT read(uint8_t *dst, uint32_t sector_start, uint32_t num_sectors) override {
		if (!sd.detect_card()) {
			_status = Status::NoCard;
			return RES_NOTRDY;
		}

		const uint32_t size_bytes = num_sectors * sd.BlockSize;
		auto ok = sd.read({dst, size_bytes}, sector_start);
		return ok ? RES_OK : RES_ERROR;
	}

	DRESULT write(const uint8_t *src, uint32_t sector_start, uint32_t num_sectors) override {
		if (!sd.detect_card()) {
			_status = Status::NoCard;
			return RES_NOTRDY;
		}

		const uint32_t size_bytes = num_sectors * sd.BlockSize;
		auto ok = sd.write({src, size_bytes}, sector_start);
		return ok ? RES_OK : RES_ERROR;
	}

	DRESULT ioctl(uint8_t cmd, uint8_t *buff) override {
		if (_status == Status::NotInit)
			return RES_NOTRDY;

		switch (cmd) {
			case GET_SECTOR_SIZE: // Get R/W sector size (WORD)
				*(WORD *)buff = sd.BlockSize;
				break;

			case GET_BLOCK_SIZE: // Get erase block size in unit of sector (DWORD)
				*(DWORD *)buff = 1;
				break;

			case GET_SECTOR_COUNT: {
				uint32_t num_blocks = sd.get_card_num_blocks();
				if (!num_blocks)
					return RES_NOTRDY;
				*(DWORD *)buff = num_blocks;
			} break;

			case MMC_GET_SDSTAT: {
				uint8_t mounted = sd.detect_card();
				*(uint8_t *)buff = mounted;
			} break;

			default:
				return RES_PARERR;
		}

		return RES_OK;
	}

private:
	Status _status = Status::NotInit;
};
