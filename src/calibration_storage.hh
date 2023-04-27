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
	uint32_t major_firmware_version = FirmwareMajorVersion;
	uint32_t minor_firmware_version = FirmwareMinorVersion;
	int32_t cv_calibration_offset[NumCVs] = {-Brain::CVAdcConf::bi_min_value,
											 -Brain::CVAdcConf::uni_min_value,
											 -Brain::CVAdcConf::uni_min_value,
											 -Brain::CVAdcConf::uni_min_value,
											 -Brain::CVAdcConf::uni_min_value};
	int32_t codec_adc_calibration_dcoffset[2] = {0, 0};
	int32_t codec_dac_calibration_dcoffset[2] = {0, 0};
	uint32_t led_brightness = 4;
	// float tracking_comp = 0.994f; // BB D: Sampler f723 in NAMM case
	float tracking_comp = 1.0f; // BB E: DIY Sampler f723 in NAMM case
	int32_t pitch_pot_detent_offset = 0;
	float rgbled_adjustments[3][3] = {{0, 0, 0}, {0, 0, 0}, {0, 0, 0}};
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
	void save_flash_params();
	void factory_reset();

private:
	void apply_firmware_specific_adjustments();
	void handle_updated_firmware();
};

} // namespace SamplerKit
