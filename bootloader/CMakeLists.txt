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
    "bootloader_retarget.c"
)
set_target_properties(${PROJECT_NAME} PROPERTIES OUTPUT_NAME "${PROJECT_NAME}.elf")

idf_build_get_property(include_directories INCLUDE_DIRECTORIES GENERATOR_EXPRESSION)
target_include_directories(${PROJECT_NAME} PRIVATE ${include_directories})

get_optimization_level(optimize "_BOOTLOADER")
target_compile_options(${PROJECT_NAME} PRIVATE ${optimize})
idf_build_get_property(compile_options COMPILE_OPTIONS GENERATOR_EXPRESSION)
target_compile_options(${PROJECT_NAME} PRIVATE ${compile_options})

# NOTE: BOOTLOADER_BUILD
idf_build_get_property(compile_definitions COMPILE_DEFINITIONS GENERATOR_EXPRESSION)
target_compile_definitions(${PROJECT_NAME} PRIVATE "BOOTLOADER_BUILD=1" ${compile_definitions})

target_include_directories(${PROJECT_NAME} PRIVATE
    "${IDF_PATH}/components/xtensa/include"
    "${IDF_PATH}/components/bootloader_support/include"
    "${IDF_PATH}/components/bootloader_support/private_include"
)
target_sources(${PROJECT_NAME} PRIVATE
    "${IDF_PATH}/components/hal/cache_hal.c"
    "${IDF_PATH}/components/hal/mmu_hal.c"
    "${IDF_PATH}/components/hal/mpu_hal.c"
    "${IDF_PATH}/components/esp_rom/patches/esp_rom_sys.c"
    "${IDF_PATH}/components/bootloader_support/src/bootloader_mem.c"
    "${IDF_PATH}/components/bootloader_support/src/${IDF_TARGET}/bootloader_soc.c"
)
target_link_options(${PROJECT_NAME} PRIVATE ${LINK_OPTIONS})

# ld
list(APPEND link_scripts
    "${CMAKE_CURRENT_LIST_DIR}/${IDF_TARGET}/bootloader.ld"
    "${IDF_PATH}/components/soc/esp32s3/ld/esp32s3.peripherals.ld"
    "${IDF_PATH}/components/esp_rom/esp32s3/ld/esp32s3.rom.ld"
    "${IDF_PATH}/components/esp_rom/esp32s3/ld/esp32s3.rom.api.ld"
    "${IDF_PATH}/components/esp_rom/esp32s3/ld/esp32s3.rom.version.ld"
)
target_linker_script(${PROJECT_NAME} PRIVATE "${link_scripts}")
