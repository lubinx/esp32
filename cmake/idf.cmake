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
    message(STATUS "variable ${IDF_PATH} is not set, default set to $ENV{HOME}/esp-idf")
    set(IDF_PATH "$ENV{HOME}/esp-idf")
endif()

set(CMAKE_TOOLCHAIN_FILE ${IDF_PATH}/tools/cmake/toolchain-${IDF_TARGET}.cmake)
set(SDKCONFIG "${CMAKE_CURRENT_LIST_DIR}/sdkconfig")

# set(IDF_CMAKE_PATH ${IDF_PATH}/tools/cmake)
set(IDF_CMAKE_PATH ${CMAKE_CURRENT_LIST_DIR}/cmake_idf)

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

    # get git submodules from ${IDF_PATH} if submodules was not initialized
    git_submodule_check(${IDF_PATH})

    # __build_get_idf_git_revision()
    idf_build_set_property(COMPILE_DEFINITIONS "IDF_VER=\"$ENV{IDF_VERSION}\"")

    idf_build_set_property(IDF_TARGET ${IDF_TARGET})
    idf_build_set_property(IDF_PATH ${IDF_PATH})
    idf_build_set_property(IDF_CMAKE_PATH ${IDF_CMAKE_PATH})
    idf_build_set_property(PYTHON "python")

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


# NOTE: *override*
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
        __build_resolve_and_add_req(_component_target ${component_target} ${req} __REQUIRES)
        __build_expand_requirements(${_component_target})
    endforeach()

    foreach(req ${priv_reqs})
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

function(IDF_build_application)
    message(STATUS "Building ESP-IDF components for target ${IDF_TARGET}")

    idf_build_get_property(prefix __PREFIX)
    file(GLOB component_dirs ${IDF_PATH}/components/*)

    foreach(component_dir ${component_dirs})
        if(IS_DIRECTORY ${component_dir})
            __component_dir_quick_check(is_component ${component_dir})
            if(is_component)
                __component_add(${component_dir} ${prefix})
            endif()
        endif()
    endforeach()

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
endfunction()

function(IDF_buildroot)
endfunction()

__build_init()
