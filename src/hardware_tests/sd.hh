#pragma once
#include "conf/board_conf.hh"
#include "conf/sd_conf.hh"
#include "drivers/sdcard.hh"
#include "fatfs/fat_file_io.hh"
#include "fatfs/sdcard_ops.hh"
#include "hardware_tests/util.hh"
#include "util/zip.hh"

namespace SamplerKit::HWTests
{

struct TestSDCard {
	SDCardOps<Brain::SDCardConf> sdcard_ops;
	FatFileIO sdcard{&sdcard_ops, Volume::SDCard};

	void run_fatfs_test() {
		std::string_view wbuf{"This is a \ntest of writing\n to a file."};
		bool ok = false;

		while (!ok) {
			Board::RevLED{}.set_color(Colors::off);
			Board::PlayLED{}.set_color(Colors::off);
			Board::BankLED{}.set_color(Colors::off);

			Board::RevG::set(true);
			ok = sdcard.create_file("test.txt", wbuf);
			if (!ok) {
				Board::RevR::set(true);
				Util::flash_mainbut_until_pressed();
				continue;
			}

			std::array<char, 128> rbuf;
			Board::BankG::set(true);
			auto bytes_read = sdcard.read_file("test.txt", rbuf);
			if (!bytes_read || bytes_read != wbuf.size()) {
				ok = false;
				Board::BankR::set(true);
				Util::flash_mainbut_until_pressed();
				continue;
			}

			for (auto [w, r] : zip(wbuf, rbuf)) {
				if (w != r) {
					ok = false;
					Board::RevB::set(true);
					Board::BankR::set(true);
					Util::flash_mainbut_until_pressed();
					continue;
				}
			}
		}

		// sdcard.foreach_file_with_ext(".wav", [&](std::string_view fname, uint32_t timestamp, uint32_t fsize) {
		// 	printf_("%.64s %08x %dB\n", fname.data(), timestamp, fsize);
		// 	// action(fno.fname, timestamp, fno.fsize);
		// });
	}

	void run_test() {
		auto &sd = sdcard_ops.sd;
		// mdrivlib::SDCard<Brain::SDCardConf> sd;

		std::array<uint8_t, 512> buf;
		for (auto &x : buf)
			x = 0xAA;

		// Allow user to insert a card if they forgot to
		// or try a different card
		bool ok = false;
		while (!ok) {
			Board::RevLED{}.set_color(Colors::off);
			Board::PlayLED{}.set_color(Colors::off);
			Board::BankLED{}.set_color(Colors::off);

			ok = sd.read(buf, 1024);
			if (!ok) {
				Board::RevR::set(true);
				Util::flash_mainbut_until_pressed();
				continue;
			}

			unsigned aas = 0;
			for (auto &x : buf) {
				if (x == 0xAA)
					aas++;
			}
			if (aas > 256) {
				ok = false;
				Board::BankR::set(true);
				Util::flash_mainbut_until_pressed();
				continue;
			}

			// Try a write test
			std::array<uint8_t, 512> wbuf;
			for (unsigned i = 0; auto &x : wbuf)
				x = (++i) & 0xFF;
			ok = sd.write(wbuf, 1024);
			if (!ok) {
				Board::BankB::set(true);
				Util::flash_mainbut_until_pressed();
				continue;
			}

			// Read back:
			ok = sd.read(wbuf, 1024);
			for (unsigned i = 0; auto &x : wbuf) {
				if (x != ((++i) & 0xFF))
					ok = false;
			}
			if (!ok) {
				Board::BankB::set(true);
				Board::RevR::set(true);
				Util::flash_mainbut_until_pressed();
				continue;
			}

			// Restore original data
			sd.write(buf, 1024);
		}
	}
};

} // namespace SamplerKit::HWTests
