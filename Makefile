BUILDDIR := build

all: | $(BUILDDIR)
	cmake --build $(BUILDDIR) 

$(BUILDDIR):
	cmake -B $(BUILDDIR) -GNinja

clean:
	rm -rf $(BUILDDIR)

wav:
	cmake --build build --target 723.wav

combo:
	cmake --build build --target 723-combo
