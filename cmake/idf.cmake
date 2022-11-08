get_property(__idf_env_set GLOBAL PROPERTY __IDF_ENV_SET)
if(__idf_env_set)
    return ()
endif()

set(IDF_PATH $ENV{IDF_PATH})
set(CMAKE_TOOLCHAIN_FILE ${IDF_PATH}/tools/cmake/toolchain-${IDF_TARGET}.cmake)

# set(IDF_CMAKE_PATH ${IDF_PATH}/tools/cmake)
set(IDF_CMAKE_PATH ${CMAKE_CURRENT_LIST_DIR}/cmake_idf)

set(SDKCONFIG "${CMAKE_CURRENT_LIST_DIR}/sdkconfig")

include(${IDF_CMAKE_PATH}/build.cmake)
include(${IDF_CMAKE_PATH}/component.cmake)
include(${IDF_CMAKE_PATH}/git_submodules.cmake)
include(${IDF_CMAKE_PATH}/kconfig.cmake)
include(${IDF_CMAKE_PATH}/ldgen.cmake)
include(${IDF_CMAKE_PATH}/utilities.cmake)
include(${IDF_CMAKE_PATH}/version.cmake)

# depgraph.cmake
    function(depgraph_add_edge)
    endfunction()
# depgraph.cmake

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

function(__build_init)  # *override*
    # idf_build_set_property() / idf_build_get_property() TARGET
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
    else()      # Set components required by all other components in the build
        set(requires_common freertos cxx newlib freertos heap log soc hal esp_rom esp_common esp_system esp_hw_support)
    endif()

    idf_build_set_property(__COMPONENT_REQUIRES_COMMON "${requires_common}")

# need by __component_get_requirements()
    # __build_set_default(PROJECT_DIR ${CMAKE_SOURCE_DIR})
    # __build_set_default(PROJECT_NAME ${CMAKE_PROJECT_NAME})
    # __build_set_default(PROJECT_VER 1)
    # __build_set_default(BUILD_DIR ${CMAKE_BINARY_DIR})
endfunction()

function(__build_expand_requirements component_target)  # *override*
    message("=== ${component_target}")

    # forward
    ___build_expand_requirements(${component_target})
endfunction()

function(IDF_glob_default_components)
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
endfunction()

__build_init()
