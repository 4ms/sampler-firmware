#include "calibration_storage.hh"

// TODO
namespace SamplerKit
{
void CalibrationStorage::factory_reset() {}
uint32_t CalibrationStorage::load_flash_params() { return 0; }
void CalibrationStorage::save_flash_params(uint8_t num_led_blinks) {}
void CalibrationStorage::update_flash_params_version() {
	apply_firmware_specific_adjustments();
	copy_system_calibrations_into_staging();
	set_firmware_version();
	write_all_system_calibrations_to_FLASH();
}

void CalibrationStorage::apply_firmware_specific_adjustments() {}
void CalibrationStorage::copy_system_calibrations_into_staging() {}
void CalibrationStorage::set_firmware_version() {}
void CalibrationStorage::write_all_system_calibrations_to_FLASH() {}

} // namespace SamplerKit
