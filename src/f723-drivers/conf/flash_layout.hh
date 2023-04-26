#pragma once

#define APP_FLASH_ADDR 0x0800C000
#define APP_START_ADDR 0x0800C000

#ifdef __cplusplus
#include <cstdint>

constexpr uint32_t NumFlashSectors = 8;

constexpr uint32_t get_sector_addr(uint32_t sector_num) {
	constexpr uint32_t F72xxE_SECTORS[] = {
		0x08000000, // Sector 0: 16k
		0x08004000, // Sector 1: 16k
		0x08008000, // Sector 2: 16k
		0x0800C000, // Sector 3: 16k
		0x08010000, // Sector 4: 64k
		0x08020000, // Sector 5: 128k
		0x08040000, // Sector 6: 128k
		0x08060000, // Sector 7: 128k
		0x08080000, // end
	};
	return (sector_num <= NumFlashSectors) ? F72xxE_SECTORS[sector_num] : 0;
}

constexpr inline uint32_t BootloaderFlashAddr = get_sector_addr(0);	  // 32k Bootloader
constexpr inline uint32_t SettingsFlashAddr = get_sector_addr(2);	  // 16k Settings
constexpr inline uint32_t AppFlashAddr = get_sector_addr(3);		  // 208k = 128k+64k+16k app
constexpr inline uint32_t BootloaderReceiveAddr = get_sector_addr(6); // 256k to receive

constexpr inline uint32_t AppStartAddr = AppFlashAddr;
constexpr inline uint32_t BootloaderStartAddr = BootloaderFlashAddr;

// Tests

// static_assert(APP_FLASH_ADDR == AppFlashAddr);
// static_assert(APP_START_ADDR == AppStartAddr);

// static_assert(get_sector_num(1) == std::nullopt);
// static_assert(get_sector_num(0) == std::nullopt);
// static_assert(get_sector_num(0x08000000) == 0);
// static_assert(get_sector_num(0x08000001) == std::nullopt);
// static_assert(get_sector_num(0x08004000) == 1);
// static_assert(get_sector_num(0x08008000) == 2);
// static_assert(get_sector_num(0x08060000) == 7);
// static_assert(get_sector_num(0x08080000) == std::nullopt);

// static_assert(get_containing_sector_num(1) == std::nullopt);
// static_assert(get_containing_sector_num(0) == std::nullopt);
// static_assert(get_containing_sector_num(0x07FFFFFF) == std::nullopt);
// static_assert(get_containing_sector_num(0x08000000) == 0);
// static_assert(get_containing_sector_num(0x08000001) == 0);
// static_assert(get_containing_sector_num(0x08004000) == 1);
// static_assert(get_containing_sector_num(0x08008000) == 2);
// static_assert(get_containing_sector_num(0x08060000) == 7);
// static_assert(get_containing_sector_num(0x0807FFFF) == 7);
// static_assert(get_containing_sector_num(0x08080000) == std::nullopt);
#endif
