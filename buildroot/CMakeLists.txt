# The following lines of boilerplate have to be in your project's
# CMakeLists in this exact order for cmake to work correctly
cmake_minimum_required(VERSION 3.5)

set(IDF_TARGET "esp32s3")
set(SDKCONFIG "${CMAKE_CURRENT_LIST_DIR}/sdkconfig")
include(idf.cmake)

project(buildroot)

set(BOOTLOADER_BUILD 1)
idf_build_set_property(COMPILE_DEFINITIONS "BOOTLOADER_BUILD=1" APPEND)

function(IDF_component_add COMPONENT_NAME) # NOTE: *override* add optional args: prefix dir
    # prefix
    list(POP_FRONT ARGN prefix)
    if (NOT prefix)
        idf_build_get_property(prefix __PREFIX)
    endif()

    if (${COMPONENT_NAME} MATCHES "${IDF_PATH}/components*")
        set(COMPONENT_DIR ${COMPONENT_NAME})
        get_filename_component(COMPONENT_NAME ${COMPONENT_DIR} NAME_WE)

        set(COMPONENT_BIN_DIR "${CMAKE_BINARY_DIR}/esp-idf/${COMPONENT_NAME}")
    else()
        list(POP_FRONT ARGN dir)
        if (NOT dir)
            set(dir "${IDF_PATH}/components")
        endif()

        set(COMPONENT_DIR "${dir}/${COMPONENT_NAME}")
        set(COMPONENT_BIN_DIR "${CMAKE_BINARY_DIR}/${COMPONENT_NAME}")
    endif()

    message(STATUS "Register Components: ${COMPONENT_ALIAS}")
    __component_add(${COMPONENT_DIR} ${prefix})

    # variable for calling add_subdirectory()
    #   COMPONENT_DIR / COMPONENT_BIN_DIR
    #   COMPONENT_NAME
    #   COMPONENT_ALIAS
    #   COMPONENT_TARGET:
    #       # NOTE COMPONENT_TARGET is 3 underscores
    #   COMPONENT_LIB:
    #       # NOTE: COMPONENT_LIB is 2 underscores,
    #       validated after calling add_subdirectory() -> idf_component_register()
    # set(COMPONENT_ALIAS "${prefix}::${COMPONENT_NAME}")
    # set(COMPONENT_TARGET "___${prefix}_${COMPONENT_NAME}")


    # set(__idf_component_context 1)
    # add_subdirectory(${COMPONENT_DIR} ${COMPONENT_BIN_DIR})
    # set(__idf_component_context 0)

    # __component_get_property(reqs ${COMPONENT_TARGET} __REQUIRES)
    # __component_get_property(priv_reqs ${COMPONENT_TARGET} __PRIV_REQUIRES)

    # foreach(iter ${priv_reqs})
    #     LIST(APPEND reqs ${iter})
    # endforeach()
endfunction()

IDF_component_add("bootloader" ${PROJECT_NAME} ${CMAKE_CURRENT_LIST_DIR})
# __component_add(${CMAKE_CURRENT_LIST_DIR}/bootloader ${PROJECT_NAME})
IDF_component_add("micro-ecc" ${PROJECT_NAME} "${IDF_PATH}/components/bootloader/subproject/components")
# IDF_component_add("bootloader_support")

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

set(common_req log esp_rom esp_common esp_hw_support newlib)
idf_build_set_property(__COMPONENT_REQUIRES_COMMON "${common_req}")

# TODO: -fPIC code?
# idf_build_set_property(C_COMPILE_OPTIONS "-fPIC" APPEND)

IDF_buildroot()
