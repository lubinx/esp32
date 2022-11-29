#############################################################################
# 💡 environment variables
#############################################################################
if (NOT IDF_TARGET)
    set(IDF_TARGET "esp32s3" CACHE STRING "esp-idf build target")
    message("❓ variable IDF_TARGET is not set, default set to esp32s3")
endif()
set_property(CACHE IDF_TARGET PROPERTY STRINGS esp32 esp32s2 esp32s3 esp32c3 esp32h2 esp32c2 esp32c6)
# some project_include.cmake direct reference this value
set(target ${IDF_TARGET})

if("${IDF_TARGET}" STREQUAL "linux")
    set(IDF_TARGET_ARCH "")
elseif(("${IDF_TARGET}" STREQUAL "esp32") OR ("${IDF_TARGET}" STREQUAL "esp32s2") OR ("${IDF_TARGET}" STREQUAL "esp32s3"))
    set(IDF_TARGET_ARCH "xtensa")
else()
    set(IDF_TARGET_ARCH "riscv")
endif()

if (NOT IDF_PATH)
    if (DEFINED ENV{IDF_PATH})
        set(IDF_PATH $ENV{IDF_PATH})
    else()
        set(IDF_PATH "$ENV{HOME}/esp-idf")
        message("❓ variable IDF_PATH is not set, default set to $ENV{HOME}/esp-idf")
    endif()
    set(IDF_PATH ${IDF_PATH} CACHE STRING "esp-idf source path")
elseif (DEFINED ENV{IDF_PATH} AND NOT ($ENV{IDF_PATH} STREQUAL ${IDF_PATH}))
    message("❌ IDF_PATH was changed since last build")
    message("✔️ clear cmake cache to fix\n\n")
    message(FATAL_ERROR)
endif()

if (NOT IDF_ENV_PATH)
    set(IDF_ENV_PATH $ENV{IDF_ENV_PATH})
endif()
if (NOT IDF_ENV_PATH)
    set(IDF_ENV_PATH "$ENV{HOME}/.espressif")
    message("❓ variable IDF_ENV_PATH is not set, default set to ${IDF_ENV_PATH}")
endif()

set(IDF_CMAKE_PATH ${IDF_PATH}/tools/cmake)
# set(IDF_CMAKE_PATH ${CMAKE_CURRENT_LIST_DIR}/../cmake_idf)

#############################################################################
# 💡 compiler toolchain variables
#############################################################################
set(CMAKE_TOOLCHAIN_FILE ${IDF_PATH}/tools/cmake/toolchain-${IDF_TARGET}.cmake)

# COMPILE_OPTIONS
list(APPEND COMPILE_OPTIONS
    "-ffunction-sections"
    "-fdata-sections"
    "-Wall"
    "-Werror=all"
    "-Wno-error=unused-function"
    "-Wno-error=deprecated-declarations"
    "-Wextra"
)

# extra COMPILE_OPTIONS for build esp-idf components only
list(APPEND COMPONENT_COMPILE_OPTIONS
    "-Wno-enum-conversion"
    "-Wno-format"
    "-Wno-sign-compare"
    "-Wno-unused-parameter"
    # "-Wno-unused-variable"
)

# C_COMPILE_OPTIONS
list(APPEND C_COMPILE_OPTIONS
    "-std=gnu17"
)

# CXX_COMPILE_OPTIONS
list(APPEND CXX_COMPILE_OPTIONS
    "-std=gnu++20"
)

# ASM_COMPILE_OPTIONS
list(APPEND ASM_COMPILE_OPTIONS
    ""
)

# COMPILE_DEFINITIONS
list(APPEND COMPILE_DEFINITIONS
    "_GNU_SOURCE"
    "ESP_PLATFORM"          # 3party components porting
)

# LINK_OPTIONS
list(APPEND LINK_OPTIONS
   ""
)

list(APPEND OBSOLETED_COMPONENTS
    # merge into esp_system
    "esp_common"
    # removed
    "esp_app_format"
    "partition_table"
)

#############################################################################
# 💡 include
#############################################################################
include(${IDF_CMAKE_PATH}/version.cmake)

# cmake last build idf version
if (IDF_BUILD_VERSION AND NOT ($ENV{IDF_VERSION} STREQUAL "${IDF_BUILD_VERSION}"))
    message("❌ IDF_VERSION was changed since last build")
    message("✔️ this is not a fatal, but recommended clear cmake cache\n\n")
else()
    set(IDF_BUILD_VERSION $ENV{IDF_VERSION} CACHE STRING "esp-idf version")
endif()
list(APPEND COMPILE_DEFINITIONS
    "IDF_VER=\"$ENV{IDF_VERSION}\""
)

# cmake caching python fullpath
if (NOT PYTHON_ENV)
    # search python: narrow to idfx.x_* should be only 1 result
    file(GLOB PYTHON_ENV "${IDF_ENV_PATH}/python_env/idf${IDF_VERSION_MAJOR}.${IDF_VERSION_MINOR}_*")
    set(PYTHON_ENV "${PYTHON_ENV}/bin/python" CACHE STRING "esp-idf python path")
endif()

include(${IDF_CMAKE_PATH}/utilities.cmake)
include(${IDF_CMAKE_PATH}/kconfig.cmake)
include(${IDF_CMAKE_PATH}/ldgen.cmake)

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
        message("💡 Checking ESP-IDF components submodules, this will only execute once")

        include(${IDF_CMAKE_PATH}/git_submodules.cmake)
        git_submodule_check(${IDF_PATH})

        message("")
    endif()
endfunction()
__git_submodule_check_once()

#############################################################################
#   idf_build_get_property()
#   idf_build_set_property()
#   idf_component_get_property() / idf_component_get_property()
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

function(__component_get_property var component_target property)
    get_property(val TARGET ${component_target} PROPERTY ${property})
    set(${var} "${val}" PARENT_SCOPE)
endfunction()

function(__component_set_property component_target property val)
    cmake_parse_arguments(_ "APPEND" "" "" ${ARGN})
    if(__APPEND)
        set_property(TARGET ${component_target} APPEND PROPERTY ${property} "${val}")
    else()
        set_property(TARGET ${component_target} PROPERTY ${property} "${val}")
    endif()
endfunction()

function(__component_get_target var component_name)
    set(${var} "__${component_name}" PARENT_SCOPE)
endfunction()

function(idf_component_get_property var component property)
    cmake_parse_arguments(_ "GENERATOR_EXPRESSION" "" "" ${ARGN})
    __component_get_target(component_target ${component})

    if(__GENERATOR_EXPRESSION)
        set(val "$<TARGET_PROPERTY:${component_target},${property}>")
    else()
        __component_get_property(val ${component_target} ${property})
    endif()
    set(${var} "${val}" PARENT_SCOPE)
endfunction()

function(idf_component_set_property component property val)
    cmake_parse_arguments(_ "APPEND" "" "" ${ARGN})
    __component_get_target(component_target ${component})

    if(__APPEND)
        __component_set_property(${component_target} ${property} "${val}" APPEND)
    else()
        __component_set_property(${component_target} ${property} "${val}")
    endif()
endfunction()

# inherited param COMPONENT_LIB
#   inherited form idf_component_register()
# param type
#   PRIVATE/PUBLIC/INTERFACE"
# ...reqs
function(idf_component_optional_requires type)
    set(reqs ${ARGN})
    idf_build_get_property(build_components BUILD_COMPONENTS)

    foreach(req ${reqs})
        if(req IN_LIST build_components)
            idf_component_get_property(req_lib ${req} COMPONENT_LIB)
            target_link_libraries(${COMPONENT_LIB} ${type} ${req_lib})
        endif()
    endforeach()
endfunction()

#############################################################################
# build initialization
#############################################################################
function(__idf_build_init)
    message("💡 ESP-IDF build initialize")
        message("\tComponents path: ${IDF_PATH}")
        message("\tTools path: ${IDF_ENV_PATH}")
        message("\tBuilding target: ${IDF_TARGET}")
        message("")

    # setup for idf_build_set_property() / idf_build_get_property()
    add_library(__idf_build_target STATIC IMPORTED GLOBAL)

    # esp-idf environments
    idf_build_set_property(IDF_PATH ${IDF_PATH})
    idf_build_set_property(IDF_TARGET ${IDF_TARGET})
    idf_build_set_property(IDF_TARGET_ARCH ${IDF_TARGET_ARCH})

    # esp-idf compiler options
    # __build_get_idf_git_revision()
    list(REMOVE_DUPLICATES COMPILE_DEFINITIONS)
    list(REMOVE_DUPLICATES COMPILE_OPTIONS)
    list(REMOVE_DUPLICATES C_COMPILE_OPTIONS)
    list(REMOVE_DUPLICATES CXX_COMPILE_OPTIONS)
    idf_build_set_property(COMPILE_DEFINITIONS "${COMPILE_DEFINITIONS}")
    idf_build_set_property(COMPILE_OPTIONS "${COMPILE_OPTIONS}")
    idf_build_set_property(C_COMPILE_OPTIONS "${C_COMPILE_OPTIONS}")
    idf_build_set_property(CXX_COMPILE_OPTIONS "${CXX_COMPILE_OPTIONS}")

    # python
    idf_build_set_property(__CHECK_PYTHON 0)        # do not check python
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
        idf_build_set_property(__COMPONENT_REQUIRES_COMMON ${IDF_TARGET_ARCH} APPEND)
    endif()

    list(APPEND common_requires
        "esp_system" "esp_rom"  "esp_hw_support"
        "freertos" "cxx" "soc" "hal" "newlib" "heap" "partition_table"
        "esptool_py"
    )
    idf_build_set_property(__COMPONENT_REQUIRES_COMMON "${common_requires}" APPEND)

    # TODO: common requires add here
endfunction()

# 💡 build initialization
__idf_build_init()

#############################################################################
# idf_component_add() / idf_component_register()
#############################################################################
function(idf_component_add name_or_dir) # optional: prefix
    # prefix
    list(POP_FRONT ARGN prefix)

    if (IS_DIRECTORY ${name_or_dir})
        get_filename_component(component_dir ${name_or_dir} ABSOLUTE BASE_DIR ${CMAKE_CURRENT_LIST_DIR})
        get_filename_component(__parent_dir ${component_dir} DIRECTORY)
        get_filename_component(component_name ${component_dir} NAME)
    else()
        set(component_dir "${IDF_PATH}/components/${name_or_dir}")
        set(__parent_dir "${IDF_PATH}/components")
        set(component_name ${name_or_dir})
    endif()

    if (${__parent_dir} STREQUAL "${IDF_PATH}/components")
        if (NOT prefix)
            idf_build_get_property(prefix __PREFIX)
        endif()
    else()
        if (NOT prefix)
            set(prefix ${PROJECT_NAME})
        endif()
    endif()

    __component_get_target(component_target ${component_name})
    set(component_lib ${prefix}_${component_name})

    idf_build_get_property(component_targets __COMPONENT_TARGETS)
    if(NOT component_target IN_LIST component_targets)
        if(NOT EXISTS "${component_dir}/CMakeLists.txt")
            message(FATAL_ERROR "Directory '${component_dir}' does not contain a component.")
        endif()

        idf_build_set_property(__COMPONENT_TARGETS ${component_target} APPEND)
        # kconfig using __BUILD_COMPONENT_TARGETS = __COMPONENT_TARGETS
        idf_build_set_property(__BUILD_COMPONENT_TARGETS ${component_target} APPEND)
        # some esp-idf components/CMakeLists.txt
        idf_build_set_property(BUILD_COMPONENTS ${component_name} APPEND)
    else()
        message(WARNING "Components ${component_name} was already added.")
        return()
    endif()

    # TODO: sub components
    # if (EXISTS "${component_dir}/components")
    # endif()
    add_library(${component_target} STATIC IMPORTED)

    # Set the basic properties of the component
    __component_set_property(${component_target} __PREFIX ${prefix})
    __component_set_property(${component_target} COMPONENT_NAME ${component_name})
    __component_set_property(${component_target} COMPONENT_LIB ${component_lib})
    __component_set_property(${component_target} COMPONENT_DIR ${component_dir})
    # build dir
    __component_set_property(${component_target} COMPONENT_BUILD_DIR "${CMAKE_BINARY_DIR}/${prefix}/${component_name}")

    # Set Kconfig related properties on the component
    __kconfig_component_init(${component_target})
    # set BUILD_COMPONENT_DIRS build property
    idf_build_set_property(BUILD_COMPONENT_DIRS ${component_dir} APPEND)

    # ⚓ call macro idf_component_register()
    include(${component_dir}/CMakeLists.txt)
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
                message(FATAL_ERROR "Component ${component_name} only supports targets: ${__REQUIRED_IDF_TARGETS}")
            endif()
        endif()

        get_property(depends GLOBAL PROPERTY DEPENDED_COMPONENTS)
        foreach(iter ${__REQUIRES} ${__PRIV_REQUIRES})
            if (NOT iter IN_LIST OBSOLETED_COMPONENTS)
                if (NOT iter IN_LIST depends)
                    set_property(GLOBAL PROPERTY DEPENDED_COMPONENTS ${iter} APPEND)
                endif()
            else()
                list(REMOVE_ITEM __REQUIRES ${iter})
                list(REMOVE_ITEM __PRIV_REQUIRES ${iter})
            endif()
        endforeach()

        __component_set_property(${component_target} REQUIRES "${__REQUIRES}")
        __component_set_property(${component_target} PRIV_REQUIRES "${__PRIV_REQUIRES}")
        __component_set_property(${component_target} KCONFIG "${__KCONFIG}" APPEND)
        __component_set_property(${component_target} KCONFIG_PROJBUILD "${__KCONFIG_PROJBUILD}" APPEND)

        message(STATUS "Add Component: ${prefix}::${component_name}")
        message("\tcomponent dir: ${component_dir}")
        if (__REQUIRES OR __PRIV_REQUIRES)
            if (__REQUIRES)
                message("\tdepends: ${__REQUIRES}")
            endif()
            if (__PRIV_REQUIRES)
                message("\tprivate depends: ${__PRIV_REQUIRES}")
            endif()
        else()
            message("\tno dependencies")
        endif()

        # ⚓tick is here, macro will cause caller to return
        return()
    else()
        __inherited_idf_component_register()
    endif()
endmacro()

function(__inherited_idf_component_register)
    # Create the final target for the component. This target is the target that is
    # visible outside the build system.
    __component_get_property(component_lib ${component_target} COMPONENT_LIB)

    # Add component manifest to the list of dependencies
    set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${component_dir}/idf_component.yml")

    idf_build_get_property(include_directories INCLUDE_DIRECTORIES GENERATOR_EXPRESSION)
        include_directories("${include_directories}")
    idf_build_get_property(compile_options COMPILE_OPTIONS GENERATOR_EXPRESSION)
        add_compile_options("${compile_options}")
    idf_build_get_property(compile_definitions COMPILE_DEFINITIONS GENERATOR_EXPRESSION)
        add_compile_definitions("${compile_definitions}")
    idf_build_get_property(c_compile_options C_COMPILE_OPTIONS GENERATOR_EXPRESSION)

    foreach(option ${c_compile_options})
        add_compile_options($<$<COMPILE_LANGUAGE:C>:${option}>)
    endforeach()

    idf_build_get_property(cxx_compile_options CXX_COMPILE_OPTIONS GENERATOR_EXPRESSION)
    foreach(option ${cxx_compile_options})
        add_compile_options($<$<COMPILE_LANGUAGE:CXX>:${option}>)
    endforeach()

    idf_build_get_property(asm_compile_options ASM_COMPILE_OPTIONS GENERATOR_EXPRESSION)
    foreach(option ${asm_compile_options})
        add_compile_options($<$<COMPILE_LANGUAGE:ASM>:${option}>)
    endforeach()

    # Glob sources
    if(__SRC_DIRS)
        foreach(dir ${__SRC_DIRS})
            get_filename_component(dir ${dir} ABSOLUTE BASE_DIR ${CMAKE_CURRENT_LIST_DIR})
            if(NOT IS_DIRECTORY ${dir})
                continue()
            endif()

            file(GLOB dir_sources "${dir}/*.c" "${dir}/*.cpp" "${dir}/*.S")
            if(dir_sources)
                list(APPEND sources ${dir_sources})
            else()
                message(WARNING "No source files found for SRC_DIRS entry '${dir}'.")
            endif()
        endforeach()
    endif()
    if(__SRCS)
        foreach(src ${__SRCS})
            get_filename_component(src ${src} ABSOLUTE BASE_DIR ${CMAKE_CURRENT_LIST_DIR})
            list(APPEND sources ${src})
        endforeach()
    endif()
    if(__EXCLUDE_SRCS)
        foreach(src ${__EXCLUDE_SRCS})
            get_filename_component(src ${src} ABSOLUTE BASE_DIR ${CMAKE_CURRENT_LIST_DIR})
            list(REMOVE_ITEM sources ${src})
        endforeach()
    endif()
    list(REMOVE_DUPLICATES sources)

    idf_build_get_property(config_dir CONFIG_DIR)
    idf_build_get_property(common_reqs __COMPONENT_REQUIRES_COMMON)
    idf_build_get_property(component_targets __COMPONENT_TARGETS)

    macro(__component_add_include_dirs lib dirs type)
        foreach(dir ${dirs})
            get_filename_component(dir ${dir} ABSOLUTE BASE_DIR ${CMAKE_CURRENT_LIST_DIR})
            if(NOT IS_DIRECTORY ${dir})
                message(FATAL_ERROR "Include directory '${dir}' is not a directory.")
            endif()
            target_include_directories(${lib} ${type} ${dir})
        endforeach()
    endmacro()

    macro(__component_set_dependencies reqs type)
        foreach(req ${reqs})
            idf_component_get_property(req_lib ${req} COMPONENT_LIB)
            target_link_libraries(${component_lib} ${type} ${req_lib})
        endforeach()
    endmacro()

    if(sources OR __EMBED_FILES OR __EMBED_TXTFILES)
        add_library(${component_lib} STATIC ${sources})
        set_target_properties(${component_lib} PROPERTIES OUTPUT_NAME ${component_name} LINKER_LANGUAGE C)

        list(REMOVE_DUPLICATES COMPONENT_COMPILE_OPTIONS)
        foreach(option ${COMPONENT_COMPILE_OPTIONS})
            target_compile_options(${component_lib} PRIVATE ${option})
        endforeach()

        __component_add_include_dirs(${component_lib} "${config_dir}" PUBLIC)
        __component_add_include_dirs(${component_lib} "${__INCLUDE_DIRS}" PUBLIC)
        __component_add_include_dirs(${component_lib} "${__PRIV_INCLUDE_DIRS}" PRIVATE)

        __component_get_property(reqs ${component_target} REQUIRES)
        list(APPEND reqs ${common_reqs})
            list(REMOVE_DUPLICATES reqs)
            list(REMOVE_ITEM reqs ${component_name})
        __component_set_dependencies("${reqs}" PUBLIC)

        __component_get_property(priv_reqs ${component_target} PRIV_REQUIRES)
        __component_set_dependencies("${priv_reqs}" PRIVATE)

        __ldgen_add_component(${component_lib})
    else()
        add_library(${component_lib} INTERFACE)

        __component_add_include_dirs(${component_lib} "${config_dir}" INTERFACE)
        __component_add_include_dirs(${component_lib} "${__INCLUDE_DIRS}" INTERFACE)

        __component_get_property(reqs ${component_target} REQUIRES)
        __component_set_dependencies("${reqs}" INTERFACE)
    endif()


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

    # Fill in the rest of component property
    __component_set_property(${component_target} SRCS "${sources}")
    __component_set_property(${component_target} INCLUDE_DIRS "${__INCLUDE_DIRS}")

    # __component_set_property(${component_target} LDFRAGMENTS "${__LDFRAGMENTS}")
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

#############################################################################
# idf_build()
#############################################################################
function(idf_build)
    message("\n💡 Resolve dependencies")

    # Generate compile_commands.json
    set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

    # add esp-idf common required components
    idf_build_get_property(common_requires __COMPONENT_REQUIRES_COMMON)
    foreach(iter ${common_requires})
        idf_build_get_property(component_targets __COMPONENT_TARGETS)
        __component_get_target(req_target ${iter})

        if (NOT req_target IN_LIST component_targets)
            idf_component_add(${iter})
        endif()
    endforeach()
    # find & add all depended components
    while(1)
        get_property(deps GLOBAL PROPERTY DEPENDED_COMPONENTS)
        list(POP_FRONT deps iter)
        set_property(GLOBAL PROPERTY DEPENDED_COMPONENTS ${deps})

        if (iter)
            idf_build_get_property(component_targets __COMPONENT_TARGETS)
            __component_get_target(req_target ${iter})

            if (NOT req_target IN_LIST component_targets)
                idf_component_add(${iter})
            endif()
        else()
            break()
        endif()
    endwhile()

    message("\n💡 Kconfig")
    # Generate sdkconfig.h/sdkconfig.cmake
    idf_build_get_property(sdkconfig SDKCONFIG)
    idf_build_get_property(sdkconfig_defaults SDKCONFIG_DEFAULTS)
    __kconfig_generate_config("${sdkconfig}" "${sdkconfig_defaults}")
    # Include the sdkconfig cmake file
    idf_build_get_property(sdkconfig_cmake SDKCONFIG_CMAKE)
    include(${sdkconfig_cmake})

    idf_build_get_property(component_targets __COMPONENT_TARGETS)
    # project_include.cmake
    foreach(component_target ${component_targets})
        __component_get_property(component_dir ${component_target} COMPONENT_DIR)

        if(EXISTS ${component_dir}/project_include.cmake)
            include(${component_dir}/project_include.cmake)
        endif()
    endforeach()

    message("💡 Link dependencies")

    # import esp-idf misc compiler options
    #   but...prevent __BUILD_COMPONENT_TARGETS auto add_subdirectory()
    idf_build_set_property(__BUILD_COMPONENT_TARGETS "")
    add_subdirectory(${IDF_PATH} ${CMAKE_BINARY_DIR}/esp-idf)

    set(__idf_component_context 1)
    enable_language(C CXX ASM)

    idf_build_get_property(build_dir BUILD_DIR)
    foreach(component_target ${component_targets})
        __component_get_property(prefix ${component_target} __PREFIX)
        __component_get_property(component_name ${component_target} COMPONENT_NAME)
        __component_get_property(COMPONENT_DIR ${component_target} COMPONENT_DIR)

        add_subdirectory(${COMPONENT_DIR} ${build_dir}/${prefix}/${component_name})
    endforeach()
    unset(__idf_component_context)

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

    foreach(component_target ${component_targets})
        __component_get_property(whole_archive ${component_target} WHOLE_ARCHIVE)
        __component_get_property(build_component ${component_target} COMPONENT_LIB)

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
    # append global link options
    list(APPEND link_options "${LINK_OPTIONS}")
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

    # done
    message("")
endfunction()
