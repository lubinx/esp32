get_property(__idf_env_set GLOBAL PROPERTY __IDF_ENV_SET)
if(__idf_env_set)
    return ()
endif()

if (NOT IDF_PATH)
    set(IDF_PATH $ENV{IDF_PATH})
endif()
if (NOT IDF_TARGET)
    set(IDF_TARGET "esp32s3")
    message(STATUS "### variable IDF_TARGET is not set, default set to esp32s3")
endif()

if (NOT IDF_PATH)
    set(IDF_PATH $ENV{IDF_PATH})
endif()
if (NOT IDF_PATH)
    set(IDF_PATH "$ENV{HOME}/esp-idf")
    message(STATUS "### variable IDF_PATH is not set, default set to $ENV{HOME}/esp-idf")
endif()

if (NOT IDF_ENV_PATH)
    set(IDF_ENV_PATH $ENV{IDF_ENV_PATH})
endif()
if (NOT IDF_ENV_PATH)
    message(STATUS "### variable IDF_ENV_PATH is not set, default set to $ENV{HOME}/.espressif")
    set(IDF_ENV_PATH "$ENV{HOME}/.espressif")
endif()

set(IDF_TARGET ${IDF_TARGET} CACHE STRING "esp-idf build target")
set(IDF_PATH ${IDF_PATH} CACHE STRING "esp-idf source path")
set(IDF_ENV_PATH ${IDF_ENV_PATH} CACHE STRING "esp-idf environment tools install path")

set_property(CACHE IDF_TARGET PROPERTY STRINGS esp32 esp32s2 esp32s3 esp32c3 esp32h2 esp32c2 esp32c6)

if (DEFINED ENV{IDF_TARGET})
    set(IDF_TARGET $ENV{IDF_TARGET})
endif()
if (DEFINED ENV{IDF_PATH})
    set(IDF_PATH $ENV{IDF_PATH})
endif()
if (DEFINED ENV{IDF_ENV_PATH})
    set(IDF_ENV_PATH $ENV{IDF_ENV_PATH})
endif()

set(CMAKE_TOOLCHAIN_FILE ${IDF_PATH}/tools/cmake/toolchain-${IDF_TARGET}.cmake)
# set(IDF_CMAKE_PATH ${IDF_PATH}/tools/cmake)
set(IDF_CMAKE_PATH ${CMAKE_CURRENT_LIST_DIR}/../cmake_idf)

include(${IDF_CMAKE_PATH}/build.cmake)
include(${IDF_CMAKE_PATH}/component.cmake)
include(${IDF_CMAKE_PATH}/git_submodules.cmake)
include(${IDF_CMAKE_PATH}/kconfig.cmake)
include(${IDF_CMAKE_PATH}/ldgen.cmake)
include(${IDF_CMAKE_PATH}/utilities.cmake)
include(${IDF_CMAKE_PATH}/version.cmake)

# function(foobar var)
#     set(${var} "foobar" PARENT_SCOPE)
#     set_property(GLOBAL PROPERTY gvar "FOOBAR set gvar")
# endfunction()

# function(foo)
#     # set(b "foo")
#     foobar(test)
#     message("${test}")
#     set(b ${b})
# endfunction()

# foo(test)

# get_property(v GLOBAL PROPERTY gvar)
# message(STATUS ${v})


#############################################################################
# overrides component.cmake
#############################################################################
function(__component_get_requirements)
endfunction()

#############################################################################
# overrides depgraph.cmake
#############################################################################
function(depgraph_add_edge)
endfunction()

#############################################################################
# overrides kconfig.cmake
#   always generate all components config
#############################################################################
function(__kconfig_generate_config)
    idf_build_get_property(component_dirs BUILD_COMPONENT_DIRS)
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

function(__build_init)
    # get git submodules from ${IDF_PATH} if submodules was not initialized
    git_submodule_check(${IDF_PATH})

    # setup for idf_build_set_property() / idf_build_get_property()
    add_library(__idf_build_target STATIC IMPORTED GLOBAL)

    idf_build_set_property(IDF_TARGET ${IDF_TARGET})
    idf_build_set_property(IDF_PATH ${IDF_PATH})
    idf_build_set_property(IDF_CMAKE_PATH ${IDF_CMAKE_PATH})

    idf_build_set_property(PROJECT_DIR ${CMAKE_SOURCE_DIR})
    idf_build_set_property(BUILD_DIR ${CMAKE_BINARY_DIR})

    idf_build_set_property(SDKCONFIG "${CMAKE_SOURCE_DIR}/sdkconfig") # from PROJECT_DIR
    idf_build_set_property(SDKCONFIG_DEFAULTS "")

    if("${IDF_TARGET}" STREQUAL "esp32" OR "${IDF_TARGET}" STREQUAL "esp32s2" OR "${IDF_TARGET}" STREQUAL "esp32s3")
        idf_build_set_property(IDF_TARGET_ARCH "xtensa")
    elseif("${IDF_TARGET}" STREQUAL "linux")
        idf_build_set_property(IDF_TARGET_ARCH "")
    else()
        idf_build_set_property(IDF_TARGET_ARCH "riscv")
    endif()

    # __build_get_idf_git_revision()
    idf_build_set_property(COMPILE_DEFINITIONS "IDF_VER=\"$ENV{IDF_VERSION}\"")

    # this is for build esp-idf components
    idf_build_set_property(__PREFIX "idf")

    # search python: narrow to idfx.x_* should be only 1 result
    file(GLOB python_dir "${IDF_ENV_PATH}/python_env/idf${IDF_VERSION_MAJOR}.${IDF_VERSION_MINOR}_*")
    idf_build_set_property(PYTHON "${python_dir}/bin/python")

    __build_set_default_build_specifications()
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
endfunction()

# NOTE: build initialization
__build_init()

function(__gathering_idf_components_deps)
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

    message(STATUS "Building ESP-IDF components for target ${IDF_TARGET}")

    file(GLOB component_dirs ${IDF_PATH}/components/*)
    idf_build_get_property(prefix  __PREFIX)

    foreach(component_dir ${component_dirs})
        if(IS_DIRECTORY ${component_dir})
            __component_dir_quick_check(is_component ${component_dir})
            if(is_component)
                __component_add(${component_dir} ${prefix})
            endif()
        endif()
    endforeach()

    # redirect call
    ___component_get_requirements()
endfunction()

function(__kconfig_idf_components)
    idf_build_get_property(sdkconfig SDKCONFIG)
    idf_build_get_property(sdkconfig_defaults SDKCONFIG_DEFAULTS)

    # temporary set all components to build
    idf_build_get_property(component_targets __COMPONENT_TARGETS)
    idf_build_set_property(__BUILD_COMPONENT_TARGETS "${component_targets}" APPEND)

    # redirect call
    ___kconfig_generate_config("${sdkconfig}" "${sdkconfig_defaults}")

    # reset build components
    idf_build_unset_property(__BUILD_COMPONENT_TARGETS)
endfunction()

function(IDF_build_application)
    __gathering_idf_components_deps()
    __kconfig_idf_components()

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

    IDF_build_application()
endmacro()
