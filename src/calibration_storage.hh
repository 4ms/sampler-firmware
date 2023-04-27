#pragma once
#include "brain_conf.hh"
#include "conf/flash_layout.hh"
#include "elements.hh"
#include "flash_block.hh"
#include "util/wear_level.hh"
#include <cstdint>

namespace SamplerKit
{

struct CalibrationData {
	uint32_t major_firmware_version;
	uint32_t minor_firmware_version;
	int32_t cv_calibration_offset[NumCVs];
	int32_t codec_adc_calibration_dcoffset[2];
	int32_t codec_dac_calibration_dcoffset[2];
	uint32_t led_brightness;
	float tracking_comp;
	int32_t pitch_pot_detent_offset;
	float rgbled_adjustments[3][3];

	bool validate() {
		return (major_firmware_version < 100) && (minor_firmware_version < 100) &&
			   (major_firmware_version + minor_firmware_version > 0) && (tracking_comp > 0.5f) &&
			   (tracking_comp < 1.5f);
	}
};

struct CalibrationStorage {
	WearLevel<FlashBlock<SettingsFlashAddr, CalibrationData, 8>> flash;
	CalibrationData cal_data;

	CalibrationStorage();
	bool save_flash_params();
	void factory_reset();

private:
	void apply_firmware_specific_adjustments();
	void handle_updated_firmware();
	void set_default_cal();
};

} // namespace SamplerKit
