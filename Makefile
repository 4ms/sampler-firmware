BUILDDIR := build

rebuild: | $(BUILDDIR)
	cmake --build $(BUILDDIR)

debug:
	cmake --build $(BUILDDIR) --config Debug

release:
	cmake --build $(BUILDDIR) --config RelWithDebInfo

$(BUILDDIR):
	cmake -B $(BUILDDIR) -G"Ninja Multi-Config" 

clean:
	rm -rf $(BUILDDIR)
