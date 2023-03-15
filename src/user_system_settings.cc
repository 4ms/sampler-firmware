#include "user_system_settings.hh"

// TODO

void UserSystemSettings::factory_reset() {}
uint32_t UserSystemSettings::load_flash_params() { return 0; }
void UserSystemSettings::save_flash_params(uint8_t num_led_blinks) {}
void UserSystemSettings::update_flash_params_version() {
	apply_firmware_specific_adjustments();
	copy_system_calibrations_into_staging();
	set_firmware_version();
	write_all_system_calibrations_to_FLASH();
}

void UserSystemSettings::apply_firmware_specific_adjustments() {}
void UserSystemSettings::copy_system_calibrations_into_staging() {}
void UserSystemSettings::set_firmware_version() {}
void UserSystemSettings::write_all_system_calibrations_to_FLASH() {}
