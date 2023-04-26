#include "calibration_storage.hh"
#include "conf/flash_layout.hh"
#include "flash.hh"
#include <bit>
#include <span>

namespace SamplerKit
{

void CalibrationStorage::factory_reset() {
	cal_data.major_firmware_version = FirmwareMajorVersion;
	cal_data.minor_firmware_version = FirmwareMinorVersion;
	cal_data.cv_calibration_offset[0] = -Brain::CVAdcConf::bi_min_value;
	cal_data.cv_calibration_offset[1] = -Brain::CVAdcConf::uni_min_value;
	cal_data.cv_calibration_offset[2] = -Brain::CVAdcConf::uni_min_value;
	cal_data.cv_calibration_offset[3] = -Brain::CVAdcConf::uni_min_value;
	cal_data.cv_calibration_offset[4] = -Brain::CVAdcConf::uni_min_value;
	cal_data.codec_adc_calibration_dcoffset[0] = 0;
	cal_data.codec_adc_calibration_dcoffset[1] = 0;
	cal_data.codec_dac_calibration_dcoffset[0] = 0;
	cal_data.codec_dac_calibration_dcoffset[1] = 0;
	cal_data.led_brightness = 4;
	cal_data.tracking_comp = 1.f;
	cal_data.pitch_pot_detent_offset = 0;
	for (int i = 0; i < 3; i++)
		for (int j = 0; j < 3; j++)
			cal_data.rgbled_adjustments[i][j] = 0;

	save_flash_params();
}

CalibrationStorage::CalibrationStorage() {
	if (!flash.read(cal_data) || !cal_data.validate()) {
		cal_data = CalibrationData{}; // default init
		flash.write(cal_data);
	}
	handle_updated_firmware();
}

void CalibrationStorage::save_flash_params() { flash.write(cal_data); }

void CalibrationStorage::handle_updated_firmware() {
	if (cal_data.major_firmware_version == FirmwareMajorVersion &&
		cal_data.minor_firmware_version == FirmwareMinorVersion)
		return;

	apply_firmware_specific_adjustments();
	cal_data.major_firmware_version = FirmwareMajorVersion;
	cal_data.minor_firmware_version = FirmwareMinorVersion;
	save_flash_params();
}

void CalibrationStorage::apply_firmware_specific_adjustments() {
	if (cal_data.major_firmware_version == 0 && cal_data.minor_firmware_version == 0) {
		// v0.0 => newer
	}
}

} // namespace SamplerKit
