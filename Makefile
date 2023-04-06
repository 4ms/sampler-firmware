BUILDDIR := build

rebuild: | $(BUILDDIR)
	cmake --build $(BUILDDIR) --config Debug 
	cmake --build $(BUILDDIR) --config RelWithDebInfo

debug:
	cmake --build $(BUILDDIR) --config Debug

release:
	cmake --build $(BUILDDIR) --config RelWithDebInfo

$(BUILDDIR):
	cmake -B $(BUILDDIR) -G"Ninja Multi-Config" 

clean:
	rm -rf $(BUILDDIR)

flash:
	python3 uimg_header.py build/mp153/RelWithDebInfo/mp153.bin build/mp153/RelWithDebInfo/mp153.uimg
	$(info ------------------------------------------------------------------------)
	$(info Reboot the board with the jumper installed before executing this command)
	$(info ------------------------------------------------------------------------)
	dfu-util -a 0 -s 0x70080000 -D build/mp153/RelWithDebInfo/mp153.uimg

