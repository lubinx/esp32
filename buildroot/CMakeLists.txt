# The following lines of boilerplate have to be in your project's
# CMakeLists in this exact order for cmake to work correctly
cmake_minimum_required(VERSION 3.5)

set(BOOTLOADER_BUILD 1)
include(idf.cmake)

project(buildroot)

macro(IDF_buildroot)
    idf_component_add("${CMAKE_CURRENT_LIST_DIR}/log")
    idf_component_add("${CMAKE_CURRENT_LIST_DIR}/bootloader")
    idf_component_add("${IDF_PATH}/components/bootloader/subproject/components/micro-ecc")
    idf_component_add("${CMAKE_CURRENT_LIST_DIR}/obsoleted/esp_app_format")
    # idf_component_add("${CMAKE_CURRENT_LIST_DIR}/obsoleted/partition_table")

    idf_build()
endmacro()

IDF_buildroot()

# # TODO: -fPIC code?
# idf_build_set_property(C_COMPILE_OPTIONS "-fPIC" APPEND)
