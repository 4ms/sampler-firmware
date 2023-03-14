cmake_minimum_required(VERSION 3.15)
set(CMAKE_TOOLCHAIN_FILE ${CMAKE_CURRENT_LIST_DIR}/cmake/arm-none-eabi-gcc.cmake)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
project(
  mp153
  C
  CXX
  ASM
)

message("Configuring MP153")
include(${CMAKE_SOURCE_DIR}/cmake/common.cmake)

set(target mp153)
set(FAMILY_NAME stm32mp1)
set(PROJECT_DRIVER_DIR ${CMAKE_SOURCE_DIR}/src/mp153-drivers)
set(MDRIVLIB_TARGET_DIR ${CMAKE_SOURCE_DIR}/lib/mdrivlib/target/stm32mp1_ca7)
set(MDRIVLIB_TARGET_DIR_2 ${CMAKE_SOURCE_DIR}/lib/mdrivlib/target/stm32mp1)
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
    -funsafe-math-optimizations
)

set(ARCH_DEFINES STM32MP157Cxx STM32MP1 CORE_CA7)

set(TARGET_INCLUDES ${MDRIVLIB_TARGET_DIR_2})
set(TARGET_SOURCES
    mmu_ca7.c
    aux_core_main.cc
    ${CMAKE_SOURCE_DIR}/lib/mdrivlib/target/stm32mp1_ca7/boot/irq_ctrl.c
    ${MDRIVLIB_TARGET_DIR_2}/drivers/sai_tdm.cc
    ${MDRIVLIB_TARGET_DIR}/drivers/interrupt_handler.cc
    ${MDRIVLIB_TARGET_DIR}/boot/startup.s
    ${MDRIVLIB_TARGET_DIR}/boot/system_init.c
)

set(WAV_ENCODE_PYTHON_CMD
    # cmake-format: off
    python ${CMAKE_SOURCE_DIR}/src/bootloader/stm_audio_bootloader/fsk/encoder.py -s 22050 -b 16 -n 8 -z 4 -p 256 -g 40960 -k 1000 $<TARGET_FILE_DIR:${target}.elf>/${target}.bin
    # cmake-format: on
)

set_target_sources_includes(
  ${PROJECT_DRIVER_DIR}
  ${MDRIVLIB_TARGET_DIR}
  ${MDRIVLIB_TARGET_DIR_2}
  ${FAMILY_NAME}
)

create_target(${target})
