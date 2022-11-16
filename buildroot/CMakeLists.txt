# The following lines of boilerplate have to be in your project's
# CMakeLists in this exact order for cmake to work correctly
cmake_minimum_required(VERSION 3.5)

include(idf.cmake)
project(buildroot)

set(BOOTLOADER_BUILD 1)

idf_component_add("${CMAKE_CURRENT_LIST_DIR}/bootloader")
idf_component_add("${IDF_PATH}/components/bootloader/subproject/components/micro-ecc")
idf_component_add("${CMAKE_CURRENT_LIST_DIR}/esp_app_format")

idf_resolve_component_depends()

# __component_add("${CMAKE_CURRENT_LIST_DIR}/bootloader" ${PROJECT_NAME} )
# __component_add("${IDF_PATH}/components/bootloader/subproject/components/micro-ecc" ${PROJECT_NAME})
# __component_add("${CMAKE_CURRENT_LIST_DIR}/esp_app_format" ${PROJECT_NAME} )

# idf_build_get_property(NAMESPACE __PREFIX)
# get_property(idf_components CACHE IDF_COMPONENTS_CACHE PROPERTY STRINGS)
# foreach(component ${idf_components})
#     __component_add("${IDF_PATH}/components/${component}" ${NAMESPACE})
# endforeach()

# # TODO: -fPIC code?
# idf_build_set_property(C_COMPILE_OPTIONS "-fPIC" APPEND)

macro(IDF_buildroot)
    set(BOOTLOADER_BUILD 1)
    idf_build_set_property(BOOTLOADER_BUILD "${BOOTLOADER_BUILD}")

    # set(requires_common cxx newlib freertos esp_hw_support heap log soc hal esp_rom esp_common esp_system)
    set(common_req log esp_rom esp_common esp_hw_support)
    idf_build_set_property(__COMPONENT_REQUIRES_COMMON "${common_req}")

    IDF_build_application()
endmacro()

IDF_buildroot()
