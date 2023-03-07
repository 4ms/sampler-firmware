rebuild:
	cmake --build build

debug:
	cmake --build build --config Debug

release:
	cmake --build build --config RelWithDebInfo

init:
	CMAKE_CONFIGURATION_TYPES="Debug;RelWithDebInfo" cmake -B build -G"Ninja Multi-Config" 
