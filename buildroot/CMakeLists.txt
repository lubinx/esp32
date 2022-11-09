# The following lines of boilerplate have to be in your project's
# CMakeLists in this exact order for cmake to work correctly
cmake_minimum_required(VERSION 3.5)

set(IDF_TARGET "esp32s3")
set(SDKCONFIG "${CMAKE_CURRENT_LIST_DIR}/sdkconfig")
include(idf.cmake)

project(buildroot)

function(IDF_component_add COMPONENT_NAME) # NOTE: *override* add optional args: prefix dir
    # prefix
    list(POP_FRONT ARGN prefix)
    if (NOT prefix)
        idf_build_get_property(prefix __PREFIX)
    endif()

    if (${COMPONENT_NAME} MATCHES "${IDF_PATH}/components*")
        set(COMPONENT_DIR ${COMPONENT_NAME})
        get_filename_component(COMPONENT_NAME ${COMPONENT_DIR} NAME_WE)
    else()
        list(POP_FRONT ARGN dir)
        if (NOT dir)
            set(dir "${IDF_PATH}/components")
        endif()

        set(COMPONENT_DIR "${dir}/${COMPONENT_NAME}")
    endif()

    __component_add(${COMPONENT_DIR} ${prefix})
endfunction()

IDF_component_add("bootloader" ${PROJECT_NAME} ${CMAKE_CURRENT_LIST_DIR})
# __component_add(${CMAKE_CURRENT_LIST_DIR}/bootloader ${PROJECT_NAME})
IDF_component_add("micro-ecc" ${PROJECT_NAME} "${IDF_PATH}/components/bootloader/subproject/components")

# A number of these components are implemented as config-only when built in the bootloader
set(COMPONENTS
    bootloader
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
    micro-ecc
    efuse
    esp_system
    newlib
)

set(BOOTLOADER_BUILD 1)
idf_build_set_property(COMPILE_DEFINITIONS "BOOTLOADER_BUILD=1" APPEND)

set(common_req log esp_rom esp_common esp_hw_support newlib)
idf_build_set_property(__COMPONENT_REQUIRES_COMMON "${common_req}")

# TODO: -fPIC code?
# idf_build_set_property(C_COMPILE_OPTIONS "-fPIC" APPEND)

IDF_build_application()
