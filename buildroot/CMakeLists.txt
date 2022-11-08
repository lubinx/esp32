# The following lines of boilerplate have to be in your project's
# CMakeLists in this exact order for cmake to work correctly
cmake_minimum_required(VERSION 3.5)

set(IDF_TARGET "esp32s3")
set(SDKCONFIG "${CMAKE_CURRENT_LIST_DIR}/sdkconfig")
include(idf.cmake)

project(buildroot)

__component_add(${CMAKE_CURRENT_LIST_DIR}/bootloader ${PROJECT_NAME})
__component_add(${IDF_PATH}/components/bootloader/subproject/components/micro-ecc ${PROJECT_NAME})

IDF_glob_default_components()

# A number of these components are implemented as config-only when built in the bootloader
set(COMPONENTS
    bootloader
    freertos
    bootloader_support
    partition_table
    esptool_py
    newlib
    hal
    soc
    log
    spi_flash
    efuse
    esp_system
    esp_hw_support
    esp_system
)

set(BOOTLOADER_BUILD 1)
idf_build_set_property(COMPILE_DEFINITIONS "BOOTLOADER_BUILD=1" APPEND)

set(common_req log esp_rom esp_common esp_hw_support newlib)
idf_build_set_property(__COMPONENT_REQUIRES_COMMON "${common_req}")

# TODO: -fPIC code?
# idf_build_set_property(C_COMPILE_OPTIONS "-fPIC" APPEND)

macro(idf_project project_name)
    message(STATUS "Building ESP-IDF components for target ${IDF_TARGET}")

    idf_build_get_property(component_targets __COMPONENT_TARGETS)
    foreach(component_target ${component_targets})
        __component_get_property(component_name ${component_target} COMPONENT_NAME)

        if(component_name IN_LIST COMPONENTS)
            list(APPEND components ${component_name})
        endif()
    endforeach()

    idf_build_process(${IDF_TARGET}
        SDKCONFIG ${SDKCONFIG}
        PROJECT_NAME ${CMAKE_PROJECT_NAME}
        PROJECT_DIR ${CMAKE_CURRENT_LIST_DIR}
        PROJECT_VER "${project_ver}"
        COMPONENTS "${components}"
    )

    set(project_elf ${CMAKE_PROJECT_NAME}.elf)

    # Create a dummy file to work around CMake requirement of having a source file while adding an
    # executable. This is also used by idf_size.py to detect the target
    set(project_elf_src ${CMAKE_BINARY_DIR}/project_elf_src_${IDF_TARGET}.c)
    add_custom_command(OUTPUT ${project_elf_src}
        COMMAND ${CMAKE_COMMAND} -E touch ${project_elf_src}
        VERBATIM)
    add_custom_target(_project_elf_src DEPENDS "${project_elf_src}")
    add_executable(${project_elf} "${project_elf_src}")
    add_dependencies(${project_elf} _project_elf_src)

    if(__PROJECT_GROUP_LINK_COMPONENTS)
        target_link_libraries(${project_elf} PRIVATE "-Wl,--start-group")
    endif()

    idf_build_get_property(build_components BUILD_COMPONENT_ALIASES)

    foreach(build_component ${build_components})
        __component_get_target(build_component_target ${build_component})
        __component_get_property(whole_archive ${build_component_target} WHOLE_ARCHIVE)

        if(whole_archive)
            message(STATUS "Component ${build_component} will be linked with -Wl,--whole-archive")
            target_link_libraries(${project_elf} PRIVATE
                "-Wl,--whole-archive"
                ${build_component}
                "-Wl,--no-whole-archive")
        else()
            target_link_libraries(${project_elf} PRIVATE ${build_component})
        endif()
    endforeach()

    if(CMAKE_C_COMPILER_ID STREQUAL "GNU")
        set(mapfile "${CMAKE_BINARY_DIR}/${CMAKE_PROJECT_NAME}.map")
        set(idf_target "${IDF_TARGET}")
        string(TOUPPER ${idf_target} idf_target)

        # Add cross-reference table to the map file
        target_link_options(${project_elf} PRIVATE "-Wl,--cref")
        # Add this symbol as a hint for idf_size.py to guess the target name
        target_link_options(${project_elf} PRIVATE "-Wl,--defsym=IDF_TARGET_${idf_target}=0")
        # Enable map file output
        target_link_options(${project_elf} PRIVATE "-Wl,--Map=${mapfile}")
        unset(idf_target)
    endif()

    idf_build_executable(${project_elf})
endmacro()


idf_project(bootloader)
