cmake_minimum_required(VERSION 3.16)

if (NOT IDF_TARGET)
    message(FATAL_ERROR "this is bootloader for esp-idf project.")
endif()


project (bootloader LANGUAGES C)
set(component_path "${IDF_PATH}/components")

add_executable(${PROJECT_NAME} "${IDF_TARGET}/entry.c")
set_target_properties(${PROJECT_NAME} PROPERTIES OUTPUT_NAME "${PROJECT_NAME}.elf")

idf_build_get_property(compile_definitions COMPILE_DEFINITIONS GENERATOR_EXPRESSION)
target_compile_definitions(${PROJECT_NAME} PRIVATE ${compile_definitions})

idf_build_get_property(compile_options COMPILE_OPTIONS GENERATOR_EXPRESSION)
target_compile_options(${PROJECT_NAME} PRIVATE ${compile_options})

idf_build_get_property(link_options LINK_OPTIONS)
target_link_options(${PROJECT_NAME} PRIVATE ${link_options})

# ld
target_link_options(${PROJECT_NAME} PRIVATE "SHELL:-T ${CMAKE_CURRENT_LIST_DIR}/${IDF_TARGET}/bootloader.ld")
target_link_options(${PROJECT_NAME} PRIVATE "SHELL:-T ${component_path}/soc/esp32s3/ld/esp32s3.peripherals.ld")
target_link_options(${PROJECT_NAME} PRIVATE "SHELL:-T ${component_path}/soc/esp32s3/ld/esp32s3.peripherals.ld")
target_link_options(${PROJECT_NAME} PRIVATE "SHELL:-T ${component_path}/esp_rom/esp32s3/ld/esp32s3.rom.ld")
target_link_options(${PROJECT_NAME} PRIVATE "SHELL:-T ${component_path}/esp_rom/esp32s3/ld/esp32s3.rom.api.ld")
target_link_options(${PROJECT_NAME} PRIVATE "SHELL:-T ${component_path}/esp_rom/esp32s3/ld/esp32s3.rom.libgcc.ld")
# target_link_options(${PROJECT_NAME} PRIVATE "SHELL:-T ${component_path}/esp_rom/esp32s3/ld/esp32s3.rom.newlib.ld")
# target_link_options(${PROJECT_NAME} PRIVATE "SHELL:-T ${component_path}/esp_rom/esp32s3/ld/esp32s3.rom.version.ld")


target_include_directories(${PROJECT_NAME} PRIVATE
    "include"
    "${component_path}/bootloader_support/include"
    "${component_path}/bootloader_support/private_include"
    "${component_path}/bootloader_support/bootloader_flash/include"
    ""
)

target_sources(${PROJECT_NAME} PRIVATE
    ""
)