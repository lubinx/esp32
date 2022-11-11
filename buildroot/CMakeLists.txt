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
    # dir
    list(POP_FRONT ARGN dir)
    if (NOT dir)
        set(dir "${IDF_PATH}/components")
    endif()

    if (${COMPONENT_NAME} MATCHES "${IDF_PATH}/components*")
        set(COMPONENT_DIR ${COMPONENT_NAME})
        get_filename_component(COMPONENT_NAME ${COMPONENT_DIR} NAME_WE)

        set(COMPONENT_BIN_DIR "${CMAKE_BINARY_DIR}/esp-idf/${COMPONENT_NAME}")
    else()
        set(COMPONENT_DIR "${dir}/${COMPONENT_NAME}")
        set(COMPONENT_BIN_DIR "${CMAKE_BINARY_DIR}/${COMPONENT_NAME}")
    endif()

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
    set(COMPONENT_ALIAS "${prefix}::${COMPONENT_NAME}")
    set(COMPONENT_TARGET "___${prefix}_${COMPONENT_NAME}")

    message(STATUS "Add component: ${COMPONENT_ALIAS}")
    # set(__idf_component_context 1)

    # set(target ${IDF_TARGET})
    # if (EXISTS ${COMPONENT_DIR}/project_include.cmake)
    #     include(${COMPONENT_DIR}/project_include.cmake)
    # endif()

    # add_subdirectory(${COMPONENT_DIR} ${COMPONENT_BIN_DIR})

    # __component_get_property(reqs ${COMPONENT_TARGET} __REQUIRES)
    # __component_get_property(priv_reqs ${COMPONENT_TARGET} __PRIV_REQUIRES)
    # set(__idf_component_context 0)

    # list(APPEND reqs ${priv_reqs})
endfunction()

# add_subdirectory(${IDF_PATH} ${CMAKE_BINARY_DIR}/esp-idf)
# set(CONFIG_APP_BUILD_GENERATE_BINARIES 1)

# NOTE: *override*
# function(__build_expand_requirements component_target)
# endfunction()

IDF_component_add("bootloader" ${PROJECT_NAME} ${CMAKE_CURRENT_LIST_DIR})
IDF_component_add("micro-ecc" ${PROJECT_NAME} "${IDF_PATH}/components/bootloader/subproject/components")
IDF_component_add("partition_table" ${PROJECT_NAME} ${CMAKE_CURRENT_LIST_DIR})

IDF_component_add("soc")
IDF_component_add("esp_rom")

IDF_component_add("xtensa")
IDF_component_add("log")
IDF_component_add("hal")
IDF_component_add("efuse")
IDF_component_add("spi_flash")
IDF_component_add("bootloader_support")
IDF_component_add("esp_app_format")
IDF_component_add("esp_common")
IDF_component_add("esp_hw_support")
IDF_component_add("esp_system")
IDF_component_add("newlib")
IDF_component_add("freertos")

IDF_component_add("esptool_py")

set(common_req log esp_rom esp_common esp_hw_support newlib)
idf_build_set_property(__COMPONENT_REQUIRES_COMMON "${common_req}")

# TODO: -fPIC code?
# idf_build_set_property(C_COMPILE_OPTIONS "-fPIC" APPEND)

IDF_build_application()
