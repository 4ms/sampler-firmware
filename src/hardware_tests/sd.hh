#pragma once
#include "conf/board_conf.hh"
#include "conf/sd_conf.hh"
#include "drivers/sdcard.hh"
#include "hardware_tests/util.hh"

namespace SamplerKit::HWTests
{

struct TestSDCard {
	mdrivlib::SDCard<Brain::SDCardConf> sd;

	void run_test() {
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
