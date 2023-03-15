#pragma once
#include <cstdint>

struct UserSystemSettings {
	void factory_reset();
	uint32_t load_flash_params();
	void save_flash_params(uint8_t num_led_blinks);
	void update_flash_params_version();

private:
	void apply_firmware_specific_adjustments();
	void copy_system_calibrations_into_staging();
	void set_firmware_version();
	void write_all_system_calibrations_to_FLASH();
};
