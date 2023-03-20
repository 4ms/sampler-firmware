#pragma once
#include "elements.hh"
#include <cstdint>

namespace SamplerKit
{

struct CalibrationStorage {
	uint32_t major_firmware_version = 0;
	uint32_t minor_firmware_version = 0;
	int32_t cv_calibration_offset[NumCVs] = {0, 0, 0, 0, 0};
	int32_t codec_adc_calibration_dcoffset[2] = {0, 0};
	int32_t codec_dac_calibration_dcoffset[2] = {0, 0};
	uint32_t led_brightness = 0;
	float tracking_comp = 0;
	int32_t pitch_pot_detent_offset = 0;
	float rgbled_adjustments[3][3] = {{0, 0, 0}, {0, 0, 0}, {0, 0, 0}};

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

} // namespace SamplerKit
