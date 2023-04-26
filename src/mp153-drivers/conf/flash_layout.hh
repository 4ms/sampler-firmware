#pragma once

#define APP_START_ADDR 0xC2000040

#ifdef __cplusplus
#include <cstdint>

constexpr uint32_t NumFlashSectors = (16 * 1024 * 1024) / (4 * 1024); // 16MB / 4k sectors

constexpr uint32_t get_sector_addr(uint32_t sector_num) {
	if (sector_num <= NumFlashSectors)
		return sector_num * 4 * 1024; // 4k sectors
	return 0;
}

constexpr inline uint32_t FSBL1FlashAddr = get_sector_addr(0);			// 0x000000
constexpr inline uint32_t FSBL2FlashAddr = get_sector_addr(64);			// 0x040000
constexpr inline uint32_t USBDFUFlashAddr = get_sector_addr(80);		// 0x050000
constexpr inline uint32_t BootloaderFlashAddr = get_sector_addr(128);	// 0x080000 + 256k
constexpr inline uint32_t AppFlashAddr = get_sector_addr(192);			// 0x0C0000 + 1M
constexpr inline uint32_t BootloaderReceiveAddr = get_sector_addr(256); // 0x100000 + 1M
constexpr inline uint32_t SettingsFlashAddr = get_sector_addr(4092);	// 0xFFC000 + 16k

constexpr inline uint32_t AppStartAddr = 0xC200'0040;
constexpr inline uint32_t BootloaderStartAddr = 0xC200'0040; //?? TODO: Audio bootloader

static_assert(APP_START_ADDR == AppStartAddr);
#endif
