#############################################################################
# 💡 environment variables
#############################################################################
if (NOT IDF_TARGET)
    set(IDF_TARGET "esp32s3" CACHE STRING "esp-idf build target")
    message(STATUS "❓ variable IDF_TARGET is not set, default set to esp32s3")
endif()
set_property(CACHE IDF_TARGET PROPERTY STRINGS esp32 esp32s2 esp32s3 esp32c3 esp32h2 esp32c2 esp32c6)
# some project_include.cmake direct reference this value
set(target ${IDF_TARGET})

if(("${IDF_TARGET}" STREQUAL "esp32") OR ("${IDF_TARGET}" STREQUAL "esp32s2") OR ("${IDF_TARGET}" STREQUAL "esp32s3"))
    set(IDF_TARGET_ARCH "xtensa")
elseif("${IDF_TARGET}" STREQUAL "linux")
    set(IDF_TARGET_ARCH "")
else()
    set(IDF_TARGET_ARCH "riscv")
endif()

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

#############################################################################
# 💡 compiler toolchain variables
#############################################################################
set(CMAKE_TOOLCHAIN_FILE ${IDF_PATH}/tools/cmake/toolchain-${IDF_TARGET}.cmake)

# COMPILE_OPTIONS
list(APPEND COMPILE_OPTIONS
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
list(REMOVE_DUPLICATES COMPILE_OPTIONS)

# C_COMPILE_OPTIONS
list(APPEND C_COMPILE_OPTIONS
    "-std=gnu17"
)
list(REMOVE_DUPLICATES C_COMPILE_OPTIONS)

# CXX_COMPILE_OPTIONS
list(APPEND CXX_COMPILE_OPTIONS
    "-std=gnu++20"
)
list(REMOVE_DUPLICATES CXX_COMPILE_OPTIONS)

# ASM_COMPILE_OPTIONS
list(APPEND ASM_COMPILE_OPTIONS
    ""
)
list(REMOVE_DUPLICATES ASM_COMPILE_OPTIONS)

# COMPILE_DEFINITIONS
list(APPEND COMPILE_DEFINITIONS
    "ESP_PLATFORM"          # 3party components porting
)
list(REMOVE_DUPLICATES COMPILE_DEFINITIONS)

# LINK_OPTIONS
list(APPEND LINK_OPTIONS
   ""
)
list(REMOVE_DUPLICATES LINK_OPTIONS)

#############################################################################
# 💡 include
#############################################################################
include(${IDF_CMAKE_PATH}/version.cmake)

# cmake last build idf version
if (IDF_BUILD_VERSION AND NOT ($ENV{IDF_VERSION} STREQUAL "${IDF_BUILD_VERSION}"))
    message(STATUS "❌ IDF_VERSION was changed since last build")
    message(STATUS "✔️ this is not a fatal, but recommended clear cmake cache\n\n")
else()
    set(IDF_BUILD_VERSION $ENV{IDF_VERSION} CACHE STRING "esp-idf version")
endif()

# cmake caching python fullpath
if (NOT PYTHON_ENV)
    # search python: narrow to idfx.x_* should be only 1 result
    file(GLOB PYTHON_ENV "${IDF_ENV_PATH}/python_env/idf${IDF_VERSION_MAJOR}.${IDF_VERSION_MINOR}_*")
    set(PYTHON_ENV "${PYTHON_ENV}/bin/python" CACHE STRING "esp-idf python path")
endif()

include(${IDF_CMAKE_PATH}/utilities.cmake)
include(${IDF_CMAKE_PATH}/kconfig.cmake)
include(${IDF_CMAKE_PATH}/ldgen.cmake)

# TODO: planning to remove this
include(${IDF_CMAKE_PATH}/__component.cmake)

# tool_version_check.cmake
function(check_expected_tool_version tool_name tool_path)
endfunction()

#############################################################################
# esp-idf git submodules
#############################################################################
function (__git_submodule_check_once)
    # get git submodules from ${IDF_PATH} if submodules was not initialized
    if (NOT GIT_SUBMODULES_CHECKED)
    set(GIT_SUBMODULES_CHECKED true CACHE BOOL "esp-idf git submodule checked")
        message(STATUS "💡 Checking ESP-IDF components submodules, this will only execute once")

        include(${IDF_CMAKE_PATH}/git_submodules.cmake)
        git_submodule_check(${IDF_PATH})

        message("")
    endif()
endfunction()
__git_submodule_check_once()

#############################################################################
# idf_build_get_property() / idf_build_set_property()
#############################################################################
function(idf_build_get_property var property)
    cmake_parse_arguments(_ "GENERATOR_EXPRESSION" "" "" ${ARGN})
    if(__GENERATOR_EXPRESSION)
        set(val "$<TARGET_PROPERTY:__idf_build_target,${property}>")
    else()
        get_property(val TARGET __idf_build_target PROPERTY ${property})
    endif()
    set(${var} ${val} PARENT_SCOPE)
endfunction()

function(idf_build_set_property property value)
    cmake_parse_arguments(_ "APPEND" "" "" ${ARGN})
    if(__APPEND)
        set_property(TARGET __idf_build_target APPEND PROPERTY ${property} ${value})
    else()
        set_property(TARGET __idf_build_target PROPERTY ${property} ${value})
    endif()
endfunction()

#############################################################################
# build initialization
#############################################################################
function(__idf_build_init)
    # setup for idf_build_set_property() / idf_build_get_property()
    add_library(__idf_build_target STATIC IMPORTED GLOBAL)

    # esp-idf environments
    idf_build_set_property(IDF_PATH ${IDF_PATH})
    idf_build_set_property(IDF_TARGET ${IDF_TARGET})
    idf_build_set_property(IDF_TARGET_ARCH ${IDF_TARGET_ARCH})

    # esp-idf compiler options
    idf_build_set_property(COMPILE_DEFINITIONS "${COMPILE_DEFINITIONS}")
    idf_build_set_property(COMPILE_OPTIONS "${COMPILE_OPTIONS}")
    idf_build_set_property(C_COMPILE_OPTIONS "${C_COMPILE_OPTIONS}")
    idf_build_set_property(CXX_COMPILE_OPTIONS "${CXX_COMPILE_OPTIONS}")

    if(BOOTLOADER_BUILD)
        idf_build_set_property(BOOTLOADER_BUILD "${BOOTLOADER_BUILD}")
        idf_build_set_property(COMPILE_DEFINITIONS "BOOTLOADER_BUILD=1" APPEND)
    endif()

    # __build_get_idf_git_revision()
    idf_build_set_property(COMPILE_DEFINITIONS "IDF_VER=\"$ENV{IDF_VERSION}\"" APPEND)

    # python
    idf_build_set_property(PYTHON "${PYTHON_ENV}")

    idf_build_set_property(PROJECT_DIR ${CMAKE_SOURCE_DIR})
    idf_build_set_property(BUILD_DIR ${CMAKE_BINARY_DIR})

    idf_build_set_property(SDKCONFIG "${CMAKE_SOURCE_DIR}/sdkconfig") # from PROJECT_DIR
    idf_build_set_property(SDKCONFIG_DEFAULTS "")

    # build esp-idf components
    idf_build_set_property(__PREFIX esp-idf)

    # kconfig
    __kconfig_init()

    # cmake cache all directory of ${IDF_PATH}/components
    if (NOT IDF_COMPONENTS_CACHE)
        set(IDF_COMPONENTS_CACHE "${IDF_PATH}/components" CACHE STRING "")
        file(GLOB dirs "${IDF_PATH}/components/*")

        foreach(dir ${dirs})
            if(IS_DIRECTORY ${dir} AND EXISTS "${dir}/CMakeLists.txt")
                get_filename_component(component_name ${dir} NAME)
                set_property(CACHE IDF_COMPONENTS_CACHE PROPERTY STRINGS ${component_name} APPEND)
            endif()
        endforeach()
    endif()

    # esp-idf components common requires
    if (NOT IDF_TARGET_ARCH STREQUAL "")
        if (NOT IDF_TARGET_ARCH IN_LIST components_resolved)
            idf_build_set_property(__COMPONENT_REQUIRES_COMMON ${IDF_TARGET_ARCH} APPEND)
        endif()
    endif()

    set(common_requires
        freertos newlib log
        # cxx
        esp_common esp_hw_support esp_rom
        esptool_py
    )
    idf_build_set_property(__COMPONENT_REQUIRES_COMMON "${common_requires}" APPEND)
    unset(common_requires)

    message("## ESP-IDF environment initialized")
    message("\tComponents path: ${IDF_PATH}")
    message("\tTools path: ${IDF_ENV_PATH}")
    message("\tBuilding target: ${IDF_TARGET}")
    message("")
endfunction()

# 💡 build initialization
__idf_build_init()

#############################################################################
# idf_component_add() / idf_component_register()
#############################################################################
function(idf_component_add COMPONENT_DIR) # optional: NAMESPACE
    # NAMESPACE
    list(POP_FRONT ARGN NAMESPACE)

    get_filename_component(COMPONENT_NAME ${COMPONENT_DIR} NAME)
    get_filename_component(__parent_dir ${COMPONENT_DIR} DIRECTORY)

    if (${__parent_dir} STREQUAL "${IDF_PATH}/components")
        if (NOT NAMESPACE)
            idf_build_get_property(NAMESPACE __PREFIX)
        endif()
    else()
        if (NOT NAMESPACE)
            set(NAMESPACE ${PROJECT_NAME})
        endif()
    endif()
    unset(__parent_dir)

    set(COMPONENT_TARGET ___${NAMESPACE}_${COMPONENT_NAME})
    set(COMPONENT_LIB ${NAMESPACE}_${COMPONENT_NAME})

    set(COMPONENT_ALIAS ${NAMESPACE}::${COMPONENT_NAME})

    get_property(components_resolved GLOBAL PROPERTY COMPONENTS_RESOLVED)
    if(NOT COMPONENT_NAME IN_LIST components_resolved)
        if(NOT EXISTS "${COMPONENT_DIR}/CMakeLists.txt")
            message(FATAL_ERROR "Directory '${COMPONENT_DIR}' does not contain a component.")
        endif()
        set_property(GLOBAL PROPERTY COMPONENTS_RESOLVED ${COMPONENT_NAME} APPEND)
    else()
        message(WARNING "Components ${COMPONENT_NAME} was already added.")
        return()
    endif()

    # TODO: sub components
    # if (EXISTS "${COMPONENT_DIR}/components")
    # endif()
    add_library(${COMPONENT_TARGET} STATIC IMPORTED)

    # TODO: remove this
    idf_build_set_property(__COMPONENT_TARGETS ${COMPONENT_TARGET} APPEND)
    idf_build_set_property(__BUILD_COMPONENT_TARGETS ${COMPONENT_TARGET} APPEND)
    idf_build_set_property(BUILD_COMPONENT_ALIASES ${COMPONENT_ALIAS} APPEND)

    # Set the basic properties of the component
    __component_set_property(${COMPONENT_TARGET} __PREFIX ${NAMESPACE})
    __component_set_property(${COMPONENT_TARGET} COMPONENT_NAME ${COMPONENT_NAME})
    __component_set_property(${COMPONENT_TARGET} COMPONENT_ALIAS ${COMPONENT_ALIAS})
    __component_set_property(${COMPONENT_TARGET} COMPONENT_LIB ${COMPONENT_LIB})
    __component_set_property(${COMPONENT_TARGET} COMPONENT_DIR ${COMPONENT_DIR})
    # build dir
    __component_set_property(${COMPONENT_TARGET} COMPONENT_BUILD_DIR "${CMAKE_BINARY_DIR}/${NAMESPACE}/${COMPONENT_NAME}")

    # Set Kconfig related properties on the component
    __kconfig_component_init(${COMPONENT_TARGET})
    # set BUILD_COMPONENT_DIRS build property
    idf_build_set_property(BUILD_COMPONENT_DIRS ${COMPONENT_DIR} APPEND)

    # ⚓ call macro idf_component_register()
    include(${COMPONENT_DIR}/CMakeLists.txt)
    # ⚓ unreachable here, returned by macro expansion
endfunction()

macro(idf_component_register)
    set(options
        WHOLE_ARCHIVE
    )
    set(single_value
        KCONFIG KCONFIG_PROJBUILD
    )
    set(multi_value
        SRCS SRC_DIRS EXCLUDE_SRCS
        INCLUDE_DIRS PRIV_INCLUDE_DIRS LDFRAGMENTS REQUIRES
        PRIV_REQUIRES REQUIRED_IDF_TARGETS EMBED_FILES EMBED_TXTFILES
    )
    cmake_parse_arguments(_ "${options}" "${single_value}" "${multi_value}" ${ARGN})

    if(NOT __idf_component_context)
        if(__REQUIRED_IDF_TARGETS)
            if(NOT IDF_TARGET IN_LIST __REQUIRED_IDF_TARGETS)
                message(FATAL_ERROR "Component ${COMPONENT_NAME} only supports targets: ${__REQUIRED_IDF_TARGETS}")
            endif()
        endif()

        idf_build_get_property(common_requires __COMPONENT_REQUIRES_COMMON)
        if (common_requires)
            list(APPEND __REQUIRES "${common_requires}")
            list(REMOVE_ITEM __REQUIRES ${COMPONENT_NAME})
            list(REMOVE_DUPLICATES __REQUIRES)
        endif()
        __component_set_property(${COMPONENT_TARGET} REQUIRES "${__REQUIRES}")
        __component_set_property(${COMPONENT_TARGET} PRIV_REQUIRES "${__PRIV_REQUIRES}")
        __component_set_property(${COMPONENT_TARGET} __COMPONENT_REGISTERED 1)

        __component_set_property(${COMPONENT_TARGET} KCONFIG "${__KCONFIG}" APPEND)
        __component_set_property(${COMPONENT_TARGET} KCONFIG_PROJBUILD "${__KCONFIG_PROJBUILD}" APPEND)

        get_property(depends GLOBAL PROPERTY COMPONENTS_DEPENDS)
        foreach(iter ${__REQUIRES} ${__PRIV_REQUIRES})
            if (NOT iter IN_LIST depends)
                set_property(GLOBAL PROPERTY COMPONENTS_DEPENDS ${iter} APPEND)
            endif()
        endforeach()

        message(STATUS "Add Components: ${COMPONENT_ALIAS}")
        message("\tcomponent dir: ${COMPONENT_DIR}")
        if (__REQUIRES)
            message("\tdepends: ${__REQUIRES}")
        endif()
        if (__PRIV_REQUIRES)
            message("\tprivate depends: ${__PRIV_REQUIRES}")
        endif()
        message("")

        # ⚓tick is here, macro will cause caller to return
        return()
    else()
        __inherited_component_register()
        # _idf_component_register(${ARGV})
    endif()
endmacro()

function(__inherited_component_register)
    # Add component manifest to the list of dependencies
    set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${COMPONENT_DIR}/idf_component.yml")

    # Create the final target for the component. This target is the target that is
    # visible outside the build system.
    __component_get_property(component_lib ${component_target} COMPONENT_LIB)

    # Use generator expression so that users can append/override flags even after call to
    # idf_build_process
    idf_build_get_property(include_directories INCLUDE_DIRECTORIES GENERATOR_EXPRESSION)
    idf_build_get_property(compile_options COMPILE_OPTIONS GENERATOR_EXPRESSION)
    idf_build_get_property(compile_definitions COMPILE_DEFINITIONS GENERATOR_EXPRESSION)
    idf_build_get_property(c_compile_options C_COMPILE_OPTIONS GENERATOR_EXPRESSION)
    idf_build_get_property(cxx_compile_options CXX_COMPILE_OPTIONS GENERATOR_EXPRESSION)
    idf_build_get_property(asm_compile_options ASM_COMPILE_OPTIONS GENERATOR_EXPRESSION)
    idf_build_get_property(common_reqs ___COMPONENT_REQUIRES_COMMON)

    include_directories("${include_directories}")
    add_compile_options("${compile_options}")
    add_compile_definitions("${compile_definitions}")
    add_c_compile_options("${c_compile_options}")
    add_cxx_compile_options("${cxx_compile_options}")
    add_asm_compile_options("${asm_compile_options}")

    idf_build_get_property(config_dir CONFIG_DIR)

    # glob sources
    set(sources "")
    if(__SRCS)
        foreach(src ${__SRCS})
            get_filename_component(src "${src}" ABSOLUTE BASE_DIR ${COMPONENT_DIR})
            list(APPEND sources ${src})
        endforeach()
    endif()
    if(__SRC_DIRS)
        foreach(dir ${__SRC_DIRS})
            get_filename_component(abs_dir ${dir} ABSOLUTE BASE_DIR ${COMPONENT_DIR})

            if(NOT IS_DIRECTORY ${abs_dir})
                continue()
            endif()

            file(GLOB dir_sources "${abs_dir}/*.c" "${abs_dir}/*.cpp" "${abs_dir}/*.S")
            list(SORT dir_sources)

            if(dir_sources)
                foreach(src ${dir_sources})
                    get_filename_component(src "${src}" ABSOLUTE BASE_DIR ${COMPONENT_DIR})
                    list(APPEND sources "${src}")
                endforeach()
            else()
                message(WARNING "No source files found for SRC_DIRS entry '${dir}'.")
            endif()
        endforeach()
    endif()

    if(__EXCLUDE_SRCS)
        foreach(src ${__EXCLUDE_SRCS})
            get_filename_component(src "${src}" ABSOLUTE)
            list(REMOVE_ITEM sources "${src}")
        endforeach()
    endif()
    list(REMOVE_DUPLICATES sources)

    macro(__component_add_include_dirs lib dirs type)
        foreach(dir ${dirs})
            get_filename_component(_dir ${dir} ABSOLUTE BASE_DIR ${CMAKE_CURRENT_LIST_DIR})
            if(NOT IS_DIRECTORY ${_dir})
                message(FATAL_ERROR "Include directory '${_dir}' is not a directory.")
            endif()
            target_include_directories(${lib} ${type} ${_dir})
        endforeach()
    endmacro()

    # The contents of 'sources' is from the __component_add_sources call
    if(sources OR __EMBED_FILES OR __EMBED_TXTFILES)
        add_library(${component_lib} STATIC ${sources})
        __component_set_property(${component_target} COMPONENT_TYPE LIBRARY)
        __component_add_include_dirs(${component_lib} "${__INCLUDE_DIRS}" PUBLIC)
        __component_add_include_dirs(${component_lib} "${__PRIV_INCLUDE_DIRS}" PRIVATE)
        __component_add_include_dirs(${component_lib} "${config_dir}" PUBLIC)
        set_target_properties(${component_lib} PROPERTIES OUTPUT_NAME ${COMPONENT_NAME} LINKER_LANGUAGE C)
        __ldgen_add_component(${component_lib})
    else()
        add_library(${component_lib} INTERFACE)
        __component_set_property(${component_target} COMPONENT_TYPE CONFIG_ONLY)
        __component_add_include_dirs(${component_lib} "${__INCLUDE_DIRS}" INTERFACE)
        __component_add_include_dirs(${component_lib} "${config_dir}" INTERFACE)
    endif()

    # Alias the static/interface library created for linking to external targets.
    # The alias is the <prefix>::<component name> name.
    __component_get_property(component_alias ${component_target} COMPONENT_ALIAS)
    add_library(${component_alias} ALIAS ${component_lib})

    # Perform other component processing, such as embedding binaries and processing linker
    # script fragments
    foreach(file ${__EMBED_FILES})
        target_add_binary_data(${component_lib} "${file}" "BINARY")
    endforeach()

    foreach(file ${__EMBED_TXTFILES})
        target_add_binary_data(${component_lib} "${file}" "TEXT")
    endforeach()

    if(__LDFRAGMENTS)
        __ldgen_add_fragment_files("${__LDFRAGMENTS}")
    endif()

    # Set dependencies
    __component_set_all_dependencies()

    __component_get_property(type ${component_target} COMPONENT_TYPE)

    # Fill in the rest of component property
    __component_set_property(${component_target} SRCS "${sources}")
    __component_set_property(${component_target} INCLUDE_DIRS "${__INCLUDE_DIRS}")

    if(type STREQUAL LIBRARY)
        __component_set_property(${component_target} PRIV_INCLUDE_DIRS "${__PRIV_INCLUDE_DIRS}")
    endif()

    __component_set_property(${component_target} LDFRAGMENTS "${__LDFRAGMENTS}")
    __component_set_property(${component_target} EMBED_FILES "${__EMBED_FILES}")
    __component_set_property(${component_target} EMBED_TXTFILES "${__EMBED_TXTFILES}")
    __component_set_property(${component_target} REQUIRED_IDF_TARGETS "${__REQUIRED_IDF_TARGETS}")

    __component_set_property(${component_target} WHOLE_ARCHIVE ${__WHOLE_ARCHIVE})

    # COMPONENT_TARGET is deprecated but is made available with same function
    # as COMPONENT_LIB for compatibility.
    set(COMPONENT_TARGET ${component_lib} PARENT_SCOPE)
    # Make the COMPONENT_LIB variable available in the component CMakeLists.txt
    set(COMPONENT_LIB ${component_lib} PARENT_SCOPE)
endfunction()

function(__idf_resolve_dependencies)
    message("## Resolve ESP-IDF required components")

    idf_build_get_property(common_requires __COMPONENT_REQUIRES_COMMON)
    foreach(iter ${common_requires})
        get_property(components_resolved GLOBAL PROPERTY COMPONENTS_RESOLVED)

        if (NOT iter IN_LIST components_resolved)
            idf_component_add("${IDF_PATH}/components/${iter}")
        endif()
    endforeach()

    function(__resolve_next_deps var)
        get_property(deps GLOBAL PROPERTY COMPONENTS_DEPENDS)
        list(POP_FRONT deps dep)
        set_property(GLOBAL PROPERTY COMPONENTS_DEPENDS ${deps})

        set(${var} ${dep} PARENT_SCOPE)
    endfunction()

    while(1)
        __resolve_next_deps(iter)

        if (iter)
            get_property(components_resolved GLOBAL PROPERTY COMPONENTS_RESOLVED)

            if (NOT iter IN_LIST components_resolved)
                message("## Resolve dependency: ${iter}")
                idf_component_add("${IDF_PATH}/components/${iter}")
            endif()
        else()
            break()
        endif()
    endwhile()
endfunction()

#############################################################################
# idf_build()
#############################################################################
function(idf_build)
    # CMAKE_PROJECT_<VAR> only available after project()
    idf_build_set_property(PROJECT_NAME ${CMAKE_PROJECT_NAME})
    if (NOT CMAKE_PROJECT_VERSION)
        idf_build_set_property(PROJECT_VER 1)
    else()
        idf_build_set_property(PROJECT_VER ${CMAKE_PROJECT_VERSION})
    endif()

    __idf_resolve_dependencies()

    include(${IDF_CMAKE_PATH}/__build.cmake)
    idf_build_get_property(component_targets __COMPONENT_TARGETS)
    foreach(component_target ${component_targets})
        __build_expand_requirements(${component_target})
    endforeach()

    # Generate sdkconfig.h/sdkconfig.cmake
    idf_build_get_property(sdkconfig SDKCONFIG)
    idf_build_get_property(sdkconfig_defaults SDKCONFIG_DEFAULTS)
    __kconfig_generate_config("${sdkconfig}" "${sdkconfig_defaults}")

    # Include the sdkconfig cmake file, since the following operations require
    # knowledge of config values.
    idf_build_get_property(sdkconfig_cmake SDKCONFIG_CMAKE)
    include(${sdkconfig_cmake})

    idf_build_get_property(build_component_targets __BUILD_COMPONENT_TARGETS)

    # Include each component's project_include.cmake
    foreach(component_target ${build_component_targets})
        __component_get_property(COMPONENT_DIR ${component_target} COMPONENT_DIR)
        if(EXISTS ${COMPONENT_DIR}/project_include.cmake)
            include(${COMPONENT_DIR}/project_include.cmake)
        endif()
    endforeach()

    # idf_build_set_property(__BUILD_COMPONENT_TARGETS "")
    add_subdirectory(${IDF_PATH} ${CMAKE_BINARY_DIR}/esp-idf)

    # set(__idf_component_context 1)
    # enable_language(C CXX ASM)

    # # Add each component as a subdirectory, processing each component's CMakeLists.txt
    # foreach(component_target ${build_component_targets})
    #     idf_build_get_property(build_dir BUILD_DIR)
    #     __component_get_property(NAMESPACE ${component_target} __PREFIX)

    #     __component_get_property(COMPONENT_DIR ${component_target} COMPONENT_DIR)
    #     __component_get_property(COMPONENT_NAME ${component_target} COMPONENT_NAME)
    #     __component_get_property(COMPONENT_ALIAS ${component_target} COMPONENT_ALIAS)

    #     # message("=== ${NAMESPACE}::${COMPONENT_NAME}")
    #     # message("\tcomponent dir: ${COMPONENT_DIR}")
    #     # message("\tbuild dir: ${build_dir}/${NAMESPACE}/${COMPONENT_NAME}")
    #     # message("")

    #     add_subdirectory(${COMPONENT_DIR} ${build_dir}/${NAMESPACE}/${COMPONENT_NAME})
    # endforeach()

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

# idf_build_executable(${project_elf}) ===============================================================
    # Set additional link flags for the executable
    idf_build_get_property(link_options LINK_OPTIONS)
    set_property(TARGET ${project_elf} APPEND PROPERTY LINK_OPTIONS "${link_options}")

    # Propagate link dependencies from component library targets to the executable
    idf_build_get_property(link_depends __LINK_DEPENDS)
    set_property(TARGET ${project_elf} APPEND PROPERTY LINK_DEPENDS "${link_depends}")

    # Set the EXECUTABLE_NAME and EXECUTABLE properties since there are generator expression
    # from components that depend on it
    get_filename_component(elf_name ${project_elf} NAME_WE)
    get_target_property(elf_dir ${project_elf} BINARY_DIR)

    idf_build_set_property(EXECUTABLE_NAME ${elf_name})
    idf_build_set_property(EXECUTABLE ${project_elf})
    idf_build_set_property(EXECUTABLE_DIR "${elf_dir}")

    # Add dependency of the build target to the executable
    add_dependencies(${project_elf} __idf_build_target)
endfunction()
