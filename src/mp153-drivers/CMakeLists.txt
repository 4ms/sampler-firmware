cmake_minimum_required(VERSION 3.15)
set(CMAKE_TOOLCHAIN_FILE
    ${CMAKE_CURRENT_LIST_DIR}/cmake/arm-none-eabi-gcc.cmake)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
project(mp153 C CXX ASM)

message("Configuring MP153")
include(../../cmake/common.cmake)

set(target mp153)
set(FAMILY_NAME stm32mp1)
set(PROJECT_DRIVER_DIR ../../src/mp153-drivers)
set(MDRIVLIB_TARGET_DIR ../../lib/mdrivlib/target/stm32mp1_ca7)
set(TARGET_LINK_SCRIPT ${CMAKE_SOURCE_DIR}/linker/stm32mp15xx_ca7.ld)
set(ARCH_FLAGS
    -mcpu=cortex-a7
    -mlittle-endian
    -mfpu=neon-vfpv4
    -mfloat-abi=hard
    -mthumb-interwork
    -mno-unaligned-access
    -mtune=cortex-a7
    -mvectorize-with-neon-quad
    -funsafe-math-optimizations)

set(ARCH_DEFINES STM32MP157Cxx STM32MP1 CORE_CA7)

set(TARGET_BOOTLOADER_SOURCES
    # ../../src/bootloader/gate_input_f423.cc
    # ../../src/f423-drivers/adc.cc
    # ../../stm32/HAL/stm32f4/Src/stm32f4xx_hal_adc.c
    # ../../stm32/HAL/stm32f4/Src/stm32f4xx_hal_dma.c
)
set(TARGET_BOOTLOADER_LINK_SCRIPT
    ${CMAKE_SOURCE_DIR}/linker/STM32F723IETx_FLASH_BOOTLOADER.ld)

set(WAV_ENCODE_PYTHON_CMD
    # Note: flash sector size is 128kB, but we arbitrarily limit app size with
    # -g in order to
    # make the  wave file shorter
    # cmake-format: off
  python ${CMAKE_SOURCE_DIR}/src/bootloader/stm_audio_bootloader/fsk/encoder.py -s 22050 -b 16 -n 8 -z 4 -p 256 -g 40960 -k 1000 $<TARGET_FILE_DIR:${target}.elf>/${target}.bin
  # cmake-format: on
)

set_target_sources_includes(${PROJECT_DRIVER_DIR} ${MDRIVLIB_TARGET_DIR}
                            ${FAMILY_NAME})

create_target(${target})
