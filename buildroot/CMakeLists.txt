# The following lines of boilerplate have to be in your project's
# CMakeLists in this exact order for cmake to work correctly
cmake_minimum_required(VERSION 3.5)

set(SDKCONFIG "${CMAKE_CURRENT_LIST_DIR}/sdkconfig")
include(idf.cmake)

project(buildroot)

idf_component_add("bootloader" ${PROJECT_NAME} ${CMAKE_CURRENT_LIST_DIR})
idf_component_add("micro-ecc" ${PROJECT_NAME} "${IDF_PATH}/components/bootloader/subproject/components")

# A number of these components are implemented as config-only when built in the bootloader
set(COMPONENTS
    # bootloader
    # micro-ecc
    bootloader_support
    esptool_py
    esp_hw_support
    esp_system
    freertos
    hal
    # partition_table
    soc
    log
    spi_flash
    efuse
    esp_system
    newlib
)

set(common_req log esp_rom esp_common esp_hw_support newlib)
idf_build_set_property(__COMPONENT_REQUIRES_COMMON "${common_req}")

# TODO: -fPIC code?
# idf_build_set_property(C_COMPILE_OPTIONS "-fPIC" APPEND)

IDF_buildroot()
