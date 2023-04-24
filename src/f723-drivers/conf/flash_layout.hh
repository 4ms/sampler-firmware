#define APP_FLASH_ADDR 0x0800C000

#ifdef __cplusplus
#include <cstdint>

constexpr uint32_t NumFlashSectors = 6;

constexpr uint32_t get_sector_addr(uint32_t sector_num) {
	constexpr uint32_t F72xxx_SECTORS[] = {
		0x08000000, // Sector 0: 16k
		0x08004000, // Sector 1: 16k
		0x08008000, // Sector 2: 16k
		0x0800C000, // Sector 3: 16k
		0x08010000, // Sector 4: 64k
		0x08020000, // Sector 5: 128k
		// 0x08040000, // Sector 6: 128k // may not be present
		// 0x08060000, // Sector 7: 128k // may not be present
		0x08040000, // end
	};
	return (sector_num <= NumFlashSectors) ? F72xxx_SECTORS[sector_num] : 0;
}

constexpr inline uint32_t BootloaderFlashAddr = get_sector_addr(0); // 32k Bootloader
constexpr inline uint32_t SettingsFlashAddr = get_sector_addr(2);	// 16k Settings
constexpr inline uint32_t AppFlashAddr = get_sector_addr(3);		// 208k = 128k+64k+16k app
constexpr inline uint32_t BootloaderReceiveAddr = get_sector_addr(3);
// receive in same sector as app because we can hold all data in RAM

static_assert(APP_FLASH_ADDR == AppFlashAddr);
#endif
