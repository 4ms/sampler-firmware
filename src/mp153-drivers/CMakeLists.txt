cmake_minimum_required(VERSION 3.15)
set(CMAKE_TOOLCHAIN_FILE ${CMAKE_CURRENT_LIST_DIR}/cmake/arm-none-eabi-gcc.cmake)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
project(mp153 LANGUAGES C CXX ASM)

message("Configuring MP153")
include(${CMAKE_SOURCE_DIR}/cmake/project.cmake)

set(root ${CMAKE_SOURCE_DIR})

set(target mp153)
set(TARGET_LINK_SCRIPT ${CMAKE_SOURCE_DIR}/linker/stm32mp15xx_ca7.ld)
set(ARCH_DEFINES STM32MP157Cxx STM32MP1 CORE_CA7)
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

set(TARGET_INCLUDES
    ${root}/src/mp153-drivers/
    ${root}/lib/brainboard/mp153
    ${root}/lib/mdrivlib/target/stm32mp1_ca7
    ${root}/lib/mdrivlib/target/stm32mp1
    ${root}/lib/CMSIS/Device/ST/STM32MP1xx/Include
    ${root}/lib/CMSIS/Core_A/Include
    ${root}/lib/STM32MP1xx_HAL_Driver/Inc
)
set(TARGET_SOURCES
    ${root}/src/mp153-drivers/mmu_ca7.c
    ${root}/src/mp153-drivers/aux_core_main.cc
    ${root}/src/mp153-drivers/flash.cc
    ${root}/lib/mdrivlib/target/stm32mp1_ca7/boot/irq_ctrl.c
    ${root}/lib/mdrivlib/target/stm32mp1/drivers/sai_tdm.cc
    ${root}/lib/mdrivlib/target/stm32mp1_ca7/drivers/interrupt_handler.cc
    ${root}/lib/mdrivlib/target/stm32mp1_ca7/boot/startup.s
    ${root}/lib/mdrivlib/target/stm32mp1_ca7/boot/system_init.c
)

set_hal_sources(HAL_SOURCES stm32mp1)
create_target(${target})

add_custom_target(
  mp153-uimg ALL
  DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/${target}.elf
  COMMAND ${CMAKE_SOURCE_DIR}/scripts/uimg_header.py ${CMAKE_CURRENT_BINARY_DIR}/${target}.bin
          ${CMAKE_CURRENT_BINARY_DIR}/${target}.uimg
)

add_custom_target(
  flash-mp153
  DEPENDS mp153-uimg
  COMMAND echo "----------------------------------------------------------------------------"
  COMMAND echo "|                  Flashing MP153 board via DFU USB                        |"
  COMMAND echo "| Reboot the board with the jumper installed before executing this command |"
  COMMAND echo "----------------------------------------------------------------------------"
  COMMAND dfu-util -a 0 -s 0x70080000 -D ${CMAKE_CURRENT_BINARY_DIR}/${target}.uimg
  VERBATIM USES_TERMINAL
)
