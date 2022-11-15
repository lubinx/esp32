if (NOT IDF_TARGET)
    set(IDF_TARGET "esp32s3" CACHE STRING "esp-idf build target")
    message(STATUS "❓ variable IDF_TARGET is not set, default set to esp32s3")
endif()
set_property(CACHE IDF_TARGET PROPERTY STRINGS esp32 esp32s2 esp32s3 esp32c3 esp32h2 esp32c2 esp32c6)

if (NOT IDF_PATH)
    if (DEFINED ENV{IDF_PATH})
        set(IDF_PATH $ENV{IDF_PATH})
    else()
        set(IDF_PATH "$ENV{HOME}/esp-idf")
        message(STATUS "❓ variable IDF_PATH is not set, default set to $ENV{HOME}/esp-idf")
    endif()
    set(IDF_PATH ${IDF_PATH} CACHE STRING "esp-idf source path")
elseif (DEFINED ENV{IDF_PATH} AND NOT ($ENV{IDF_PATH} STREQUAL ${IDF_PATH}))
    message(STATUS "❌ IDF_PATH was changed since last build")
    message(STATUS "✔️ clear cmake cache to fix\n\n")
    message(FATAL_ERROR)
endif()

if (NOT IDF_ENV_PATH)
    set(IDF_ENV_PATH $ENV{IDF_ENV_PATH})
endif()
if (NOT IDF_ENV_PATH)
    set(IDF_ENV_PATH "$ENV{HOME}/.espressif")
    message(STATUS "❓ variable IDF_ENV_PATH is not set, default set to ${IDF_ENV_PATH}")
endif()

# set(IDF_CMAKE_PATH ${IDF_PATH}/tools/cmake)
set(IDF_CMAKE_PATH ${CMAKE_CURRENT_LIST_DIR}/../cmake_idf)


# toolchain & default compile options
set(CMAKE_TOOLCHAIN_FILE
    ${IDF_PATH}/tools/cmake/toolchain-${IDF_TARGET}.cmake
)
set(COMPILE_OPTIONS
    "-ffunction-sections"
    "-fdata-sections"
    # warning-related flags
    "-Wall"
    "-Werror=all"
    "-Wno-error=unused-function"
    "-Wno-error=unused-variable"
    "-Wno-error=deprecated-declarations"
    "-Wextra"
    "-Wno-unused-parameter"
    "-Wno-sign-compare"
    # ignore multiple enum conversion warnings since gcc 11
    # TODO: IDF-5163
    # "-Wno-enum-conversion"
)
set(C_COMPILE_OPTIONS
    "-std=gnu17"
)
set(CXX_COMPILE_OPTIONS
    "-std=gnu++20"
)
set(ASM_COMPILE_OPTIONS
    ""
)
set(COMPILE_DEFINITIONS
    ""
)
set(LINK_OPTIONS
   ""
)

#############################################################################
# 💡 include
#############################################################################
include(${IDF_CMAKE_PATH}/version.cmake)

if (IDF_VERSION AND NOT ($ENV{IDF_VERSION} STREQUAL "${IDF_VERSION}"))
    message(STATUS "❌ IDF_VERSION was changed since last build")
    message(STATUS "✔️ this is not a fatal, but recommended clear cmake cache\n\n")
else()
    set(IDF_VERSION $ENV{IDF_VERSION} CACHE STRING "esp-idf version")
endif()

# cmake caching python bin fullpath
if (NOT PYTHON_ENV)
    # search python: narrow to idfx.x_* should be only 1 result
    file(GLOB PYTHON_ENV "${IDF_ENV_PATH}/python_env/idf${IDF_VERSION_MAJOR}.${IDF_VERSION_MINOR}_*")
    set(PYTHON_ENV "${PYTHON_ENV}/bin/python" CACHE STRING "esp-idf python path")
endif()
include(${IDF_CMAKE_PATH}/build.cmake)
include(${IDF_CMAKE_PATH}/component.cmake)
include(${IDF_CMAKE_PATH}/git_submodules.cmake)
include(${IDF_CMAKE_PATH}/kconfig.cmake)
include(${IDF_CMAKE_PATH}/ldgen.cmake)
include(${IDF_CMAKE_PATH}/utilities.cmake)

#############################################################################
# overrides component.cmake
#############################################################################


#############################################################################
# overrides depgraph.cmake
#############################################################################
function(depgraph_add_edge)
endfunction()

#############################################################################
# overrides targets.cmake
#############################################################################
function(__target_check)
endfunction()

#############################################################################
# overrides tool_version_check.cmake
#############################################################################
function(check_expected_tool_version tool_name tool_path)
    # required by ${IDF_PATH}/components/<dir>/project_include.cmake
    #   esp_common/project_include.cmake is only SEEN using this function
endfunction()

#############################################################################
# build.cmake
#############################################################################
macro(__build_set_default)
endmacro()

macro(__build_set_default_build_specifications)
endmacro()

macro(__build_set_lang_version)
endmacro()

function(__build_init)
    # get git submodules from ${IDF_PATH} if submodules was not initialized
    git_submodule_check(${IDF_PATH})

    # setup for idf_build_set_property() / idf_build_get_property()
    add_library(__idf_build_target STATIC IMPORTED GLOBAL)

    # __build_set_default_build_specifications
    idf_build_set_property(COMPILE_DEFINITIONS "${COMPILE_DEFINITIONS}")
    idf_build_set_property(COMPILE_OPTIONS "${COMPILE_OPTIONS}")
    idf_build_set_property(C_COMPILE_OPTIONS "${C_COMPILE_OPTIONS}")
    idf_build_set_property(CXX_COMPILE_OPTIONS "${CXX_COMPILE_OPTIONS}")

    idf_build_set_property(IDF_TARGET ${IDF_TARGET})
    idf_build_set_property(IDF_PATH ${IDF_PATH})
    idf_build_set_property(IDF_CMAKE_PATH ${IDF_CMAKE_PATH})

    idf_build_set_property(PROJECT_DIR ${CMAKE_SOURCE_DIR})
    idf_build_set_property(BUILD_DIR ${CMAKE_BINARY_DIR})

    idf_build_set_property(SDKCONFIG "${CMAKE_SOURCE_DIR}/sdkconfig") # from PROJECT_DIR
    idf_build_set_property(SDKCONFIG_DEFAULTS "")

    if(("${IDF_TARGET}" STREQUAL "esp32") OR ("${IDF_TARGET}" STREQUAL "esp32s2") OR ("${IDF_TARGET}" STREQUAL "esp32s3"))
        idf_build_set_property(IDF_TARGET_ARCH "xtensa")
    elseif("${IDF_TARGET}" STREQUAL "linux")
        idf_build_set_property(IDF_TARGET_ARCH "")
    else()
        idf_build_set_property(IDF_TARGET_ARCH "riscv")
    endif()

    # __build_get_idf_git_revision()
    idf_build_set_property(COMPILE_DEFINITIONS "IDF_VER=\"${IDF_VERSION}\"")

    # python
    idf_build_set_property(PYTHON "${PYTHON_ENV}")

    # build esp-idf components
    idf_build_set_property(__PREFIX esp-idf)

    __build_set_lang_version()
    __kconfig_init()

# TODO: remove these
    if("${IDF_TARGET}" STREQUAL "linux")
        set(requires_common freertos log esp_rom esp_common linux)
    else()
        set(requires_common cxx newlib freertos esp_hw_support heap log soc hal esp_rom esp_common esp_system)
    endif()

    idf_build_set_property(__COMPONENT_REQUIRES_COMMON "${requires_common}")
####################

    # cmake cache all directory of ${IDF_PATH}/components
    if (NOT IDF_COMPONENTS_CACHE)
        set(IDF_COMPONENTS_CACHE "${IDF_PATH}/components" CACHE STRING "")
        file(GLOB dirs "${IDF_PATH}/components/*")

        foreach(dir ${dirs})
            if(IS_DIRECTORY ${dir})
                __component_dir_quick_check(is_component ${dir})
                if(is_component)
                    get_filename_component(component_name ${dir} NAME)
                    set_property(CACHE IDF_COMPONENTS_CACHE PROPERTY STRINGS ${component_name} APPEND)
                endif()
            endif()
        endforeach()
    endif()

    message(STATUS "Building ESP-IDF components for target ${IDF_TARGET}")
endfunction()

# 💡 build initialization
__build_init()
#############################################################################

function(__build_expand_requirements component_target)
    # Since there are circular dependencies, make sure that we do not infinitely
    # expand requirements for each component.
    idf_build_get_property(component_targets_seen __COMPONENT_TARGETS_SEEN)
    __component_get_property(component_registered ${component_target} __COMPONENT_REGISTERED)
    if(component_target IN_LIST component_targets_seen OR NOT component_registered)
        return()
    endif()

    idf_build_set_property(__COMPONENT_TARGETS_SEEN ${component_target} APPEND)

    get_property(reqs TARGET ${component_target} PROPERTY REQUIRES)
    get_property(priv_reqs TARGET ${component_target} PROPERTY PRIV_REQUIRES)
    __component_get_property(component_name ${component_target} COMPONENT_NAME)
    __component_get_property(component_alias ${component_target} COMPONENT_ALIAS)
    idf_build_get_property(common_reqs __COMPONENT_REQUIRES_COMMON)
    list(APPEND reqs ${common_reqs})

    if(reqs)
        list(REMOVE_DUPLICATES reqs)
        list(REMOVE_ITEM reqs ${component_alias} ${component_name})
    endif()

    foreach(req ${reqs})
        depgraph_add_edge(${component_name} ${req} REQUIRES)
        __build_resolve_and_add_req(_component_target ${component_target} ${req} __REQUIRES)
        __build_expand_requirements(${_component_target})
    endforeach()

    foreach(req ${priv_reqs})
        depgraph_add_edge(${component_name} ${req} PRIV_REQUIRES)
        __build_resolve_and_add_req(_component_target ${component_target} ${req} __PRIV_REQUIRES)
        __build_expand_requirements(${_component_target})
    endforeach()

    idf_build_get_property(build_component_targets __BUILD_COMPONENT_TARGETS)
    if(NOT component_target IN_LIST build_component_targets)
        idf_build_set_property(__BUILD_COMPONENT_TARGETS ${component_target} APPEND)

        __component_get_property(component_lib ${component_target} COMPONENT_LIB)
        idf_build_set_property(__BUILD_COMPONENTS ${component_lib} APPEND)

        idf_build_get_property(prefix __PREFIX)
        __component_get_property(component_prefix ${component_target} __PREFIX)

        __component_get_property(component_alias ${component_target} COMPONENT_ALIAS)

        idf_build_set_property(BUILD_COMPONENT_ALIASES ${component_alias} APPEND)

        # Only put in the prefix in the name if it is not the default one
        if(component_prefix STREQUAL prefix)
            __component_get_property(component_name ${component_target} COMPONENT_NAME)
            idf_build_set_property(BUILD_COMPONENTS ${component_name} APPEND)
        else()
            idf_build_set_property(BUILD_COMPONENTS ${component_alias} APPEND)
        endif()
    endif()
endfunction()
#############################################################################

function(IDF_build_application)
    # CMAKE_PROJECT_<VAR> only available after project()
    idf_build_set_property(PROJECT_NAME ${CMAKE_PROJECT_NAME})
    if (NOT CMAKE_PROJECT_VERSION)
        idf_build_set_property(PROJECT_VER 1)
    else()
        idf_build_set_property(PROJECT_VER ${CMAKE_PROJECT_VERSION})
    endif()

    if(BOOTLOADER_BUILD)
        idf_build_set_property(BOOTLOADER_BUILD "${BOOTLOADER_BUILD}")
        idf_build_set_property(COMPILE_DEFINITIONS "BOOTLOADER_BUILD=1" APPEND)
    endif()

    idf_build_get_property(NAMESPACE __PREFIX)

    get_property(idf_components CACHE IDF_COMPONENTS_CACHE PROPERTY STRINGS)
    foreach(component ${idf_components})
        __component_add("${IDF_PATH}/components/${component}" ${NAMESPACE})
    endforeach()

    idf_build_get_property(component_targets __COMPONENT_TARGETS)
    foreach(component_target ${component_targets})
        __component_get_property(component_name ${component_target} COMPONENT_NAME)

        if(component_name IN_LIST COMPONENTS)
            list(APPEND components ${component_name})
        endif()
    endforeach()

    idf_build_process(${IDF_TARGET}
        SDKCONFIG_DEFAULTS "${SDKCONFIG_DEFAULTS}"
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
            message(STATUS "🔗 Add link library: ${build_component}")
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
endfunction()

macro(IDF_buildroot)
    set(BOOTLOADER_BUILD 1)
    idf_build_set_property(BOOTLOADER_BUILD "${BOOTLOADER_BUILD}")

    set(COMPONENTS
        # partition_table
        bootloader_support
        esptool_py
        esp_hw_support
        esp_system
        freertos
        hal
        soc
        log
        spi_flash
        efuse
        esp_system
        newlib
    )

    # set(requires_common cxx newlib freertos esp_hw_support heap log soc hal esp_rom esp_common esp_system)
    set(common_req log esp_rom esp_common esp_hw_support newlib)
    idf_build_set_property(__COMPONENT_REQUIRES_COMMON "${common_req}")

    IDF_build_application()
endmacro()
