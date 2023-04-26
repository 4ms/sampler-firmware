#include "calibration_storage.hh"
#include "conf/flash_layout.hh"
#include "flash.hh"
#include <bit>
#include <span>

// TODO
namespace SamplerKit
{
void CalibrationStorage::factory_reset() {
	uint32_t major_firmware_version = FirmwareMajorVersion;
	uint32_t minor_firmware_version = FirmwareMinorVersion;
	int32_t cv_calibration_offset[NumCVs] = {-Brain::CVAdcConf::bi_min_value,
											 -Brain::CVAdcConf::uni_min_value,
											 -Brain::CVAdcConf::uni_min_value,
											 -Brain::CVAdcConf::uni_min_value,
											 -Brain::CVAdcConf::uni_min_value};
	int32_t codec_adc_calibration_dcoffset[2] = {0, 0};
	int32_t codec_dac_calibration_dcoffset[2] = {0, 0};
	uint32_t led_brightness = 0;
	float tracking_comp = 1.f;
	int32_t pitch_pot_detent_offset = 0;
	float rgbled_adjustments[3][3] = {{0, 0, 0}, {0, 0, 0}, {0, 0, 0}};
}

// Returns firmware version [major,minor] or 0 if failed
[[maybe_unused]] std::pair<uint32_t, uint32_t> CalibrationStorage::load_flash_params() {
	auto this_bits = reinterpret_cast<uint8_t *>(this);
	std::span<uint8_t> stored_data{this_bits, sizeof(CalibrationStorage)};

	if (!flash_read(stored_data, SettingsFlashAddr))
		return {0, 0};

	return {major_firmware_version, major_firmware_version};
}

bool CalibrationStorage::save_flash_params() {
	auto this_bits = reinterpret_cast<uint8_t *>(this);
	std::span<const uint8_t> stored_data{this_bits, sizeof(CalibrationStorage)};

	return flash_erase_and_write(stored_data, SettingsFlashAddr);
}

void CalibrationStorage::update_flash_params_version() {
	apply_firmware_specific_adjustments();
	major_firmware_version = FirmwareMajorVersion;
	minor_firmware_version = FirmwareMinorVersion;
	save_flash_params();
}

void CalibrationStorage::apply_firmware_specific_adjustments() {
	if (major_firmware_version == 0 && minor_firmware_version == 1) {
		//
	}
}

} // namespace SamplerKit
