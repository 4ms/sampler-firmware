#pragma once

#define APP_FLASH_ADDR 0x08018000
#define APP_START_ADDR 0x08018000

#ifdef __cplusplus
// TODO: put APP_FLASH_ADDR etc in a .h file, which is used to preprocess linker script
// include that file here so can static_assert we use the same value
#include <cstdint>

#include "flash_sectors.hh"

constexpr inline uint32_t BootloaderFlashAddr = get_sector_addr(0);	  // 32k Bootloader
constexpr inline uint32_t SettingsFlashAddr = get_sector_addr(2);	  // 16k Settings
constexpr inline uint32_t AppFlashAddr = get_sector_addr(3);		  // 208k = 128k+64k+16k app
constexpr inline uint32_t BootloaderReceiveAddr = get_sector_addr(6); // 256k to receive

constexpr inline uint32_t AppStartAddr = AppFlashAddr;
constexpr inline uint32_t BootloaderStartAddr = BootloaderFlashAddr;

static_assert(APP_FLASH_ADDR == AppFlashAddr);
static_assert(APP_START_ADDR == AppStartAddr);

// Tests

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
