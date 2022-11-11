get_property(__idf_env_set GLOBAL PROPERTY __IDF_ENV_SET)
if(__idf_env_set)
    return ()
endif()

if (NOT IDF_TARGET)
    set(IDF_TARGET $ENV{IDF_TARGET})
endif()
if (NOT IDF_TARGET)
    message(FATAL_ERROR "variable IDF_TARGET is not set: esp32 esp32s2 esp32s3 esp32c3 esp32h2 esp32c2 esp32c6")
endif()

if (NOT IDF_PATH)
    set(IDF_PATH $ENV{IDF_PATH})
endif()
if (NOT IDF_PATH)
    message(STATUS "variable IDF_PATH is not set, default set to $ENV{HOME}/esp-idf")
    set(IDF_PATH "$ENV{HOME}/esp-idf")
endif()

if (NOT IDF_ENV_PATH)
    set(IDF_ENV_PATH $ENV{IDF_ENV_PATH})
endif()
if (NOT IDF_ENV_PATH)
    message(STATUS "variable IDF_ENV_PATH is not set, default set to $ENV{HOME}/.espressif")
    set(IDF_ENV_PATH "$ENV{HOME}/.espressif")
endif()

set(CMAKE_TOOLCHAIN_FILE ${IDF_PATH}/tools/cmake/toolchain-${IDF_TARGET}.cmake)
set(SDKCONFIG "${CMAKE_CURRENT_LIST_DIR}/sdkconfig")

# set(IDF_CMAKE_PATH ${IDF_PATH}/tools/cmake)
set(IDF_CMAKE_PATH ${CMAKE_CURRENT_LIST_DIR}/../cmake_idf)

include(${IDF_CMAKE_PATH}/build.cmake)
include(${IDF_CMAKE_PATH}/component.cmake)
include(${IDF_CMAKE_PATH}/git_submodules.cmake)
include(${IDF_CMAKE_PATH}/kconfig.cmake)
include(${IDF_CMAKE_PATH}/ldgen.cmake)
include(${IDF_CMAKE_PATH}/utilities.cmake)
include(${IDF_CMAKE_PATH}/version.cmake)

# targets.cmake
    function(__target_check)
    endfunction()
# targets.cmake

# tool_version_check.cmake
    function(check_expected_tool_version tool_name tool_path)
        # required by ${IDF_PATH}/components/<dir>/project_include.cmake
        #   esp_common/project_include.cmake is only SEEN using this function
    endfunction()
# tool_version_check.cmake

# NOTE: *override*
function(__build_init)
    # setup for idf_build_set_property() / idf_build_get_property()
    add_library(__idf_build_target STATIC IMPORTED GLOBAL)

    # All targets built under this scope is with the ESP-IDF build system
    idf_build_set_property(COMPILE_DEFINITIONS "ESP_PLATFORM" APPEND)

    # __build_get_idf_git_revision()
    #   get git submodules from ${IDF_PATH} if submodules was not initialized
    git_submodule_check(${IDF_PATH})
    idf_build_set_property(COMPILE_DEFINITIONS "IDF_VER=\"$ENV{IDF_VERSION}\"")

    idf_build_set_property(IDF_TARGET ${IDF_TARGET})
    idf_build_set_property(IDF_PATH ${IDF_PATH})
    idf_build_set_property(IDF_CMAKE_PATH ${IDF_CMAKE_PATH})

    # search python: narrow to idfx.x_* should be only 1 result
    file(GLOB python_dir "${IDF_ENV_PATH}/python_env/idf${IDF_VERSION_MAJOR}.${IDF_VERSION_MINOR}_*")
    idf_build_set_property(PYTHON "${python_dir}/bin/python")

    # this is for ${IDF_PATH}/CMakeList.txt
    set(prefix "idf")
    idf_build_set_property(__PREFIX ${prefix})

    __build_set_default_build_specifications()
    __build_set_lang_version()
    __kconfig_init()

    if("${IDF_TARGET}" STREQUAL "linux")
        set(requires_common freertos log esp_rom esp_common linux)
    else()
        set(requires_common cxx newlib freertos esp_hw_support heap log soc hal esp_rom esp_common esp_system)
    endif()

    idf_build_set_property(__COMPONENT_REQUIRES_COMMON "${requires_common}")

# build defaults
    # __build_set_default(PROJECT_DIR ${CMAKE_SOURCE_DIR})
    # __build_set_default(PROJECT_NAME ${CMAKE_PROJECT_NAME})
    # __build_set_default(PROJECT_VER 1)
    # __build_set_default(BUILD_DIR ${CMAKE_BINARY_DIR})
endfunction()

macro(idf_component_register)   # NOTE: *override* for direct set __REQUIRES & __PRIV_REQUIRES
    set(multi_value REQUIRES PRIV_REQUIRES)
    cmake_parse_arguments(_ "" "" "${multi_value}" "${ARGN}")

    if (NOT COMPONENT_TARGET)
        idf_build_get_property(prefix __PREFIX)
        set(COMPONENT_TARGET "___${prefix}_${COMPONENT_NAME}")
    endif()

    _idf_component_register(${ARGV})

    if (__REQUIRES)
        __component_set_property(${COMPONENT_TARGET} __REQUIRES ${__REQUIRES})
    endif()
    if (__PRIV_REQUIRES)
        __component_set_property(${COMPONENT_TARGET} __PRIV_REQUIRES ${__PRIV_REQUIRES})
    endif()

    # macro for PARENT_SCROPE variables
    # set(COMPONENT_LIB ${COMPONENT_LIB} PARENT_SCOPE)
endmacro()

function(depgraph_add_edge)
endfunction()

function(IDF_build_application)
    # message(STATUS "Building ESP-IDF components for target ${IDF_TARGET}")
    idf_build_get_property(SDKCONFIG_DEFAULTS SDKCONFIG_DEFAULTS)
    if (EXISTS "${CMAKE_CURRENT_LIST_DIR}/sdkconfig.defaults")
        list(APPEND SDKCONFIG_DEFAULTS "${CMAKE_CURRENT_LIST_DIR}/sdkconfig.defaults")
    endif()

    # message(FATAL_ERROR "${SDKCONFIG_DEFAULTS}")

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

function(IDF_buildroot)
endfunction()

__build_init()
