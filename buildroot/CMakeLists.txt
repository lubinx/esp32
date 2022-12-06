cmake_minimum_required(VERSION 3.16)
include(../cmake/idf.cmake)

project(kernel)

# macro(IDF_buildroot)
#     idf_component_add("esp_system")
#     # idf_component_add("bootloader_support")
#     # idf_component_add("esp_hw_support")
#     # idf_component_add("spi_flash")
#     # idf_component_add("${CMAKE_CURRENT_LIST_DIR}/obsoleted/esp_common")
#     # idf_component_add("${CMAKE_CURRENT_LIST_DIR}/obsoleted/partition_table")
#     # idf_component_add("${CMAKE_CURRENT_LIST_DIR}/obsoleted/esp_app_format")

#     idf_build()
# endmacro()

# IDF_buildroot()

idf_build(
    COMPONENTS
        # ...
    SRCS
        "main.c"
)
