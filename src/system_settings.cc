#include "system_settings.hh"

// TODO

void SystemCalibrations::factory_reset() {}
uint32_t SystemCalibrations::load_flash_params() { return 0; }
void SystemCalibrations::save_flash_params(uint8_t num_led_blinks) {}
void SystemCalibrations::update_flash_params_version() {
	apply_firmware_specific_adjustments();
	copy_system_calibrations_into_staging();
	set_firmware_version();
	write_all_system_calibrations_to_FLASH();
}

void SystemCalibrations::apply_firmware_specific_adjustments() {}
void SystemCalibrations::copy_system_calibrations_into_staging() {}
void SystemCalibrations::set_firmware_version() {}
void SystemCalibrations::write_all_system_calibrations_to_FLASH() {}
