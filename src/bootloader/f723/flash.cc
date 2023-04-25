/*
 * flash.c -
 *
 * Author: Dan Green (danngreen1@gmail.com)
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 * See http://creativecommons.org/licenses/MIT/ for more information.
 *
 * -----------------------------------------------------------------------------
 */

#include "conf/flash_layout.hh"
#include "drivers/stm32xx.h"
#include <optional>
#include <span>

static void flash_begin_open_program(void);
static void flash_end_open_program(void);
static HAL_StatusTypeDef flash_open_erase_page(uint32_t address);
static HAL_StatusTypeDef flash_open_program_halfword(uint8_t halfword, uint32_t address);
static HAL_StatusTypeDef flash_open_program_word(uint32_t word, uint32_t address);
static HAL_StatusTypeDef flash_open_program_doubleword(uint64_t doubleword, uint32_t address);
static HAL_StatusTypeDef flash_open_program_byte_array(uint8_t *arr, uint32_t address, uint32_t num_bytes);
static HAL_StatusTypeDef flash_open_program_halfword_array(uint16_t *arr, uint32_t address, uint32_t num_bytes);
static HAL_StatusTypeDef flash_open_program_word_array(uint32_t *arr, uint32_t address, uint32_t num_bytes);
static HAL_StatusTypeDef flash_erase_page(uint32_t address);
static uint16_t flash_read_halfword(uint32_t address);
static uint32_t flash_read_word(uint32_t address);
static uint8_t flash_read_byte(uint32_t address);
static HAL_StatusTypeDef flash_read_byte_array(uint8_t *arr, uint32_t address, uint32_t num_bytes);
static HAL_StatusTypeDef flash_read_halfword_array(uint16_t *arr, uint32_t address, uint32_t num_bytes);
static HAL_StatusTypeDef flash_read_word_array(uint32_t *arr, uint32_t address, uint32_t num_bytes);
static HAL_StatusTypeDef flash_read_doubleword_array(uint64_t *arr, uint32_t address, uint32_t num_bytes);

HAL_StatusTypeDef _flash_erase(uint32_t address) {
	auto sector_start = get_sector_num(address);
	if (!sector_start)
		return HAL_OK;

	uint32_t sector = sector_start.value();
	FLASH_EraseInitTypeDef eraseInit;
	eraseInit.TypeErase = FLASH_TYPEERASE_SECTORS;
	eraseInit.Sector = sector;
	eraseInit.NbSectors = 1;
	eraseInit.VoltageRange = FLASH_VOLTAGE_RANGE_3;
	uint32_t result = 0;
	HAL_StatusTypeDef status = HAL_FLASHEx_Erase(&eraseInit, &result);
	if (result != 0xFFFFFFFF)
		return HAL_ERROR;
	else {
		return status;
	}
}

void flash_begin_open_program(void) { HAL_FLASH_Unlock(); }
void flash_end_open_program(void) { HAL_FLASH_Lock(); }

HAL_StatusTypeDef flash_erase_page(uint32_t address) {
	HAL_StatusTypeDef status;

	HAL_FLASH_Unlock();
	status = _flash_erase(address);
	HAL_FLASH_Lock();
	return status;
}

HAL_StatusTypeDef flash_open_erase_page(uint32_t address) { return _flash_erase(address); }

HAL_StatusTypeDef flash_open_program_halfword(uint8_t halfword, uint32_t address) {
	// return FLASH_ProgramHalfWord(address, halfword);
	return HAL_ERROR; // Not for STM32G0xx
}

HAL_StatusTypeDef flash_open_program_doubleword(uint64_t doubleword, uint32_t address) {
	if (address & 0b111)
		return HAL_ERROR; // address must be double-word aligned

	return HAL_FLASH_Program(FLASH_TYPEPROGRAM_DOUBLEWORD, address, doubleword);
}

HAL_StatusTypeDef flash_open_program_word(uint32_t word, uint32_t address) {
	if (address & 0b11)
		return HAL_ERROR;

	return HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, address, word);
}

HAL_StatusTypeDef flash_open_program_byte_array(uint8_t *arr, uint32_t address, uint32_t num_bytes) {
	HAL_StatusTypeDef status = HAL_OK;

	if (!num_bytes)
		return HAL_ERROR;

	while (num_bytes--) {
		status = HAL_FLASH_Program(FLASH_TYPEPROGRAM_BYTE, address, *arr++);
		if (status != HAL_OK)
			break;
		address++;
	}
	return status;
}

HAL_StatusTypeDef flash_open_program_doubleword_array(uint64_t *arr, uint32_t address, uint32_t num_bytes) {
	HAL_StatusTypeDef status = HAL_OK;

	if (num_bytes & 0b111)
		return HAL_ERROR; // can only write doublewords: use word_array or byte_array or halfword_array

	uint32_t num_doublewords = num_bytes / 8;

	if (!num_doublewords)
		return HAL_ERROR;

	while (num_doublewords--) {
		status = flash_open_program_doubleword(*arr++, address);
		if (status != HAL_OK)
			break;
		address += 8;
	}
	return status;
}

HAL_StatusTypeDef flash_open_program_word_array(uint32_t *arr, uint32_t address, uint32_t num_bytes) {
	HAL_StatusTypeDef status = HAL_OK;

	if (address & 0b11)
		return HAL_ERROR; // address must be word-aligned

	if (num_bytes & 0b11)
		return HAL_ERROR; // can only write words: use byte_array or halfword_array

	uint32_t num_words = num_bytes / 4;

	if (!num_words)
		return HAL_ERROR;

	while (num_words--) {
		status = flash_open_program_word(*arr++, address);
		if (status != HAL_OK)
			break;
		address += 4;
	}
	return status;
}

HAL_StatusTypeDef flash_write(std::span<const uint8_t *> data, uint32_t dst_addr) {
	uint32_t bytes_to_write = data.size_bytes();
	if (!bytes_to_write)
		return HAL_OK;

	flash_begin_open_program();

	// Erase sector if dst_addr is a sector start
	auto err = flash_open_erase_page(dst_addr);
	if (err != HAL_OK) {
		flash_end_open_program();
		return err;
	}

	// Erase all sectors that start within our span of data
	auto sec = get_containing_sector_num(dst_addr);
	if (!sec)
		return HAL_ERROR;
	uint32_t sec_num = sec.value();

	auto last_sec = get_containing_sector_num(dst_addr + bytes_to_write - 1);
	if (!last_sec)
		return HAL_ERROR;
	uint32_t last_sec_num = last_sec.value();

	while (sec_num <= last_sec_num) {
		if (auto err = flash_open_erase_page(get_sector_addr(sec_num)); err != HAL_OK) {
			flash_end_open_program();
			return err;
		}
		sec_num++;
	}

	err = flash_open_program_word_array((uint32_t *)data.data(), dst_addr, bytes_to_write);
	if (err != HAL_OK) {
		flash_end_open_program();
		return err;
	}

	flash_end_open_program();
	return HAL_OK;
}

HAL_StatusTypeDef flash_write_page(const uint8_t *data, uint32_t dst_addr, uint32_t bytes_to_write) {

	flash_begin_open_program();

	// Erase sector if dst_addr is a sector start
	auto err = flash_open_erase_page(dst_addr);
	if (err != HAL_OK) {
		flash_end_open_program();
		return err;
	}

	err = flash_open_program_word_array((uint32_t *)data, dst_addr, bytes_to_write);
	if (err != HAL_OK) {
		flash_end_open_program();
		return err;
	}

	flash_end_open_program();
	return HAL_OK;
}

HAL_StatusTypeDef flash_read_byte_array(uint8_t *arr, uint32_t address, uint32_t num_bytes) {
	while (num_bytes--) {
		*arr++ = (uint8_t)(*(__IO uint8_t *)address);
		address++;
	}
	return HAL_OK;
}

HAL_StatusTypeDef flash_read_halfword_array(uint16_t *arr, uint32_t address, uint32_t num_bytes) {
	if (num_bytes & 0b1)
		return HAL_ERROR; // can only write halfwords: use byte_array

	uint32_t num_halfwords = num_bytes / 2;

	while (num_halfwords--) {
		*arr++ = (uint16_t)(*(__IO uint16_t *)address);
		address += 2;
	}
	return HAL_OK;
}

HAL_StatusTypeDef flash_read_word_array(uint32_t *arr, uint32_t address, uint32_t num_bytes) {
	if (num_bytes & 0b11)
		return HAL_ERROR; // can only write words: use byte_array or halfword_array

	uint32_t num_words = num_bytes / 4;

	while (num_words--) {
		*arr++ = (uint32_t)(*(__IO uint32_t *)address);
		address += 4;
	}
	return HAL_OK;
}

HAL_StatusTypeDef flash_read_doubleword_array(uint64_t *arr, uint32_t address, uint32_t num_bytes) {
	if (num_bytes & 0b111)
		return HAL_ERROR; // can only write doublewords: use byte_array or halfword_array or word_array

	uint32_t num_doublewords = num_bytes / 8;

	while (num_doublewords--) {
		*arr++ = (uint64_t)(*(__IO uint64_t *)address);
		address += 8;
	}
	return HAL_OK;
}

uint32_t flash_read_word(uint32_t address) { return (*(__IO uint32_t *)address); }

uint16_t flash_read_halfword(uint32_t address) { return ((uint16_t)(*(__IO uint16_t *)address)); }

uint8_t flash_read_byte(uint32_t address) { return ((uint8_t)(*(__IO uint8_t *)address)); }
