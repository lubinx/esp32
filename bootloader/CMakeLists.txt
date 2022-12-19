cmake_minimum_required(VERSION 3.16)

if (NOT IDF_TARGET)
    message(FATAL_ERROR "this is bootloader for esp-idf project.")
endif()

# NOTE: sub-project bootloader
#   direct add compile the src files without any dependency
project (bootloader LANGUAGES C)

add_executable(${PROJECT_NAME}
    "${IDF_TARGET}/startup.c"
    "bootloader_log.c"
)
set_target_properties(${PROJECT_NAME} PROPERTIES OUTPUT_NAME "${PROJECT_NAME}.elf")

idf_build_get_property(include_directories INCLUDE_DIRECTORIES GENERATOR_EXPRESSION)
target_include_directories(${PROJECT_NAME} PRIVATE ${include_directories})

# NOTE: BOOTLOADER_BUILD
idf_build_get_property(compile_definitions COMPILE_DEFINITIONS GENERATOR_EXPRESSION)
target_compile_definitions(${PROJECT_NAME} PRIVATE "BOOTLOADER_BUILD=1" ${compile_definitions})

idf_build_get_property(compile_options COMPILE_OPTIONS GENERATOR_EXPRESSION)
target_compile_options(${PROJECT_NAME} PRIVATE ${compile_options})

idf_build_get_property(link_options LINK_OPTIONS)
target_link_options(${PROJECT_NAME} PRIVATE ${link_options})

# ld
list(APPEND link_scripts
    "${CMAKE_CURRENT_LIST_DIR}/${IDF_TARGET}/bootloader.ld"
    "${IDF_PATH}/components/soc/esp32s3/ld/esp32s3.peripherals.ld"
    "${IDF_PATH}/components/esp_rom/esp32s3/ld/esp32s3.rom.ld"
    "${IDF_PATH}/components/esp_rom/esp32s3/ld/esp32s3.rom.api.ld"
    "${IDF_PATH}/components/esp_rom/esp32s3/ld/esp32s3.rom.version.ld"
)
target_linker_script(${PROJECT_NAME} PRIVATE "${link_scripts}")

target_include_directories(${PROJECT_NAME} PRIVATE
    "${IDF_PATH}/components/xtensa/include"
    "${IDF_PATH}/components/bootloader_support/include"
    "${IDF_PATH}/components/bootloader_support/private_include"
    "${IDF_PATH}/components/esp_hw_support/include/esp_private"
    "${IDF_PATH}/components/esp_hw_support/port/include"
)

target_sources(${PROJECT_NAME} PRIVATE
    "${IDF_PATH}/components/bootloader_support/src/${IDF_TARGET}/bootloader_soc.c"
    "${IDF_PATH}/components/bootloader_support/src/bootloader_clock_init.c"
    "${IDF_PATH}/components/bootloader_support/src/bootloader_mem.c"
    "${IDF_PATH}/components/esp_hw_support/cpu.c"
    "${IDF_PATH}/components/esp_hw_support/port/${IDF_TARGET}/rtc_clk.c"
    "${IDF_PATH}/components/esp_hw_support/port/${IDF_TARGET}/rtc_clk_init.c"
    "${IDF_PATH}/components/esp_hw_support/port/${IDF_TARGET}/rtc_init.c"
    "${IDF_PATH}/components/esp_rom/patches/esp_rom_sys.c"
    "${IDF_PATH}/components/esp_rom/patches/esp_rom_uart.c"
    "${IDF_PATH}/components/hal/cache_hal.c"
    "${IDF_PATH}/components/hal/mmu_hal.c"
    "${IDF_PATH}/components/hal/mpu_hal.c"
    "${IDF_PATH}/components/newlib/syscalls.c"
)