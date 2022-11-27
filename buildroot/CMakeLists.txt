# The following lines of boilerplate have to be in your project's
# CMakeLists in this exact order for cmake to work correctly
cmake_minimum_required(VERSION 3.5)
include(../cmake/idf.cmake)

project(buildroot)

macro(IDF_buildroot)
    idf_component_add("${CMAKE_CURRENT_LIST_DIR}/freertos")
    idf_component_add("${CMAKE_CURRENT_LIST_DIR}/esp_system")
    idf_component_add("${CMAKE_CURRENT_LIST_DIR}/spi_flash")
    idf_component_add("${CMAKE_CURRENT_LIST_DIR}/esp_hw_support")
    idf_component_add("${CMAKE_CURRENT_LIST_DIR}/bootloader")
    idf_component_add("${IDF_PATH}/components/bootloader/subproject/components/micro-ecc")
    idf_component_add("${CMAKE_CURRENT_LIST_DIR}/bootloader_support")

    idf_component_add("${CMAKE_CURRENT_LIST_DIR}/esptool_py")
    # idf_component_add("${CMAKE_CURRENT_LIST_DIR}/obsoleted/esp_common")
    # idf_component_add("${CMAKE_CURRENT_LIST_DIR}/obsoleted/partition_table")
    # idf_component_add("${CMAKE_CURRENT_LIST_DIR}/obsoleted/esp_app_format")

    idf_build()
endmacro()

IDF_buildroot()

# # TODO: -fPIC code?
# idf_build_set_property(C_COMPILE_OPTIONS "-fPIC" APPEND)
