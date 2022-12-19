get_property(__idf_env_set GLOBAL PROPERTY __IDF_ENV_SET)
if(NOT __idf_env_set)
    set_property(GLOBAL PROPERTY __IDF_ENV_SET 1)
else()
    return()
endif()

#############################################################################
# 💡 environment variables
#############################################################################
if (NOT IDF_TARGET)
    set(IDF_TARGET "esp32s3" CACHE STRING "esp-idf build target")
    message("❓ variable IDF_TARGET is not set, default set to esp32s3")
endif()
set_property(CACHE IDF_TARGET PROPERTY STRINGS esp32 esp32s2 esp32s3 esp32c3 esp32h2 esp32c2 esp32c6)

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

#############################################################################
# 💡 compiler toolchain variables
#############################################################################
set(CMAKE_TOOLCHAIN_FILE ${IDF_PATH}/tools/cmake/toolchain-${IDF_TARGET}.cmake)

# compile options for project source & esp-idf'components
list(APPEND COMPILE_OPTIONS
    "$<$<COMPILE_LANGUAGE:C>:-std=gnu17>"
    "$<$<COMPILE_LANGUAGE:CXX>:-std=gnu++20>"
    "-ffunction-sections"
    "-fdata-sections"
    "-Wall"
    "-Werror=all"
    "-Wextra"
    "-Wconversion"
    "-Wformat"
    "-Wundef"
    "-Wshadow"
)

# link options
list(APPEND LINK_OPTIONS
    "-Wl,--gc-sections"
    "-Wl,--warn-common"
)

# compile definitions for esp-idf'components only
list(APPEND IDF_COMPILE_DEFINITIONS
    "ESP_PLATFORM"          # 3party components porting
    "_GNU_SOURCE"
)

# extra / override compile options for esp-idf'components only
list(APPEND IDF_COMPILE_OPTIONS
    "-O3"                       # ignore Kconfig force using O3 for now
    "-Wno-array-bounds"         # freertos
    "-Wno-conversion"
    "-Wno-enum-conversion"
    "-Wno-format"
    "-Wno-shadow"
    "-Wno-sign-compare"
    "-Wno-undef"
    "-Wno-unused-parameter"
    "-Wno-unused-variable"
    "-fno-jump-tables"
    "-fno-tree-switch-conversion"
    "$<$<COMPILE_LANGUAGE:C>:-fstrict-volatile-bitfields>"
    "$<$<COMPILE_LANGUAGE:C>:-Wno-old-style-declaration>"
)

# IDF_KERNEL_COMPONENTS
#   TODO: components listed in here will generate solo sdkconfig
if (NOT IDF_TARGET_ARCH STREQUAL "")
    list(APPEND IDF_KERNEL_COMPONENTS ${IDF_TARGET_ARCH})
endif()
list(APPEND IDF_KERNEL_COMPONENTS
    "freertos" "newlib" "heap"  "cxx" "pthread"
    "esp_common" "esp_hw_support" "esp_rom" "esp_system"
    "soc" "hal" "efuse" "driver"
    "spi_flash"
    "esptool_py"
)

# OBSOLETED_COMPONENTS: autoremove from REQUIRES & PRIV_REQUIRES
list(APPEND OBSOLETED_COMPONENTS
    # merged into esp_common
    "app_update"
    "esp_app_format"
    # merge into esp_system & direct ref by bootloader
    "bootloader_support"
    # removed
    "partition_table"
)

#############################################################################
# 💡 include
#############################################################################
include(${IDF_CMAKE_PATH}/version.cmake)
include(${IDF_CMAKE_PATH}/utilities.cmake)
include(${IDF_CMAKE_PATH}/kconfig.cmake)
include(${IDF_CMAKE_PATH}/ldgen.cmake)

# cmake last build idf version
if (IDF_BUILD_VERSION AND NOT ($ENV{IDF_VERSION} STREQUAL "${IDF_BUILD_VERSION}"))
    message("❌ IDF_VERSION was changed since last build")
    message("✔️ this is not a fatal, but recommended clear cmake cache\n\n")
else()
    set(IDF_BUILD_VERSION $ENV{IDF_VERSION} CACHE STRING "esp-idf version")
endif()
list(APPEND IDF_COMPILE_DEFINITIONS "IDF_VER=\"$ENV{IDF_VERSION}\"")

# cmake caching python fullpath
if (NOT PYTHON_ENV)
    # search python: narrow to idfx.x_* should be only 1 result
    file(GLOB PYTHON_ENV "${IDF_ENV_PATH}/python_env/idf${IDF_VERSION_MAJOR}.${IDF_VERSION_MINOR}_*")
    set(PYTHON_ENV "${PYTHON_ENV}/bin/python" CACHE STRING "esp-idf python path")
endif()

# tool_version_check.cmake
function(check_expected_tool_version tool_name tool_path)
endfunction()

#############################################################################
# esp-idf git submodules
#############################################################################
function (__idf_git_submodule_check_once)
    # get git submodules from ${IDF_PATH} if submodules was not initialized
    if (NOT GIT_SUBMODULES_CHECKED)
    set(GIT_SUBMODULES_CHECKED true CACHE BOOL "esp-idf git submodule checked")
        message("💡 Checking ESP-IDF components submodules")
        include(${IDF_CMAKE_PATH}/git_submodules.cmake)
        git_submodule_check(${IDF_PATH})
        message("")
    endif()
endfunction()
__idf_git_submodule_check_once()

#############################################################################
# esp-idf global properties
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
# esp-idf component properties
#############################################################################
function(idf_get_component_target var component_name)
    set(${var} "__${component_name}" PARENT_SCOPE)
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

function(idf_component_get_property var component property)
    cmake_parse_arguments(_ "GENERATOR_EXPRESSION" "" "" ${ARGN})
    idf_get_component_target(component_target ${component})

    if(__GENERATOR_EXPRESSION)
        set(val "$<TARGET_PROPERTY:${component_target},${property}>")
    else()
        __component_get_property(val ${component_target} ${property})
    endif()
    set(${var} "${val}" PARENT_SCOPE)
endfunction()

function(idf_component_set_property component property val)
    cmake_parse_arguments(_ "APPEND" "" "" ${ARGN})
    idf_get_component_target(component_target ${component})

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
function(__build_init)
    message("💡 ESP-IDF build initialize")
        message("\tComponents path: ${IDF_PATH}")
        message("\tTools path: ${IDF_ENV_PATH}")
        message("\tBuilding target: ${IDF_TARGET}")

    # setup for idf_build_set_property() / idf_build_get_property()
    add_library(__idf_build_target STATIC IMPORTED GLOBAL)

    # esp-idf environments
    idf_build_set_property(IDF_PATH ${IDF_PATH})
    idf_build_set_property(IDF_TARGET ${IDF_TARGET})
    idf_build_set_property(IDF_TARGET_ARCH ${IDF_TARGET_ARCH})

    # python
    # idf_build_set_property(__CHECK_PYTHON 0)        # do not check python
    idf_build_set_property(PYTHON "${PYTHON_ENV}")

    idf_build_set_property(PROJECT_DIR ${CMAKE_SOURCE_DIR})
    idf_build_set_property(BUILD_DIR ${CMAKE_BINARY_DIR})

    idf_build_set_property(SDKCONFIG "${CMAKE_SOURCE_DIR}/sdkconfig") # from PROJECT_DIR
    idf_build_set_property(SDKCONFIG_DEFAULTS "")

    # kconfig
    # idf_build_set_property(IDF_PATH ${CMAKE_CURRENT_LIST_DIR})
    __kconfig_init()

    # build esp-idf components
    idf_build_set_property(__PREFIX esp-idf)

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
    idf_build_set_property(__COMPONENT_REQUIRES_COMMON "${IDF_KERNEL_COMPONENTS}" APPEND)
endfunction()

# 💡 build initialization
__build_init()

#############################################################################
# idf_target_include_directories(): fix relative path
#############################################################################
function(idf_target_include_directories component_target type dirs)
    foreach(dir ${dirs})
        get_filename_component(dir ${dir} ABSOLUTE BASE_DIR ${CMAKE_CURRENT_LIST_DIR})
        if(NOT IS_DIRECTORY ${dir})
            message(FATAL_ERROR "Include directory '${dir}' is not a directory.")
        endif()

        if (${type} STREQUAL "PUBLIC" AND component_name IN_LIST IDF_KERNEL_COMPONENTS)
            idf_build_set_property(INCLUDE_DIRECTORIES ${dir} APPEND)
        else()
            target_include_directories(${component_target} ${type} ${dir})
        endif()
    endforeach()
endfunction()

#############################################################################
# idf_component_add() / idf_component_register()
#############################################################################
function(idf_component_add name_or_dir) # optional: prefix
    # prefix
    list(POP_FRONT ARGN prefix)

    if (IS_DIRECTORY ${name_or_dir} OR IS_DIRECTORY "${CMAKE_CURRENT_LIST_DIR}/${name_or_dir}")
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

    idf_get_component_target(component_target ${component_name})
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

    # TODO: sub components ?
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
        __component_set_property(${component_target} WHOLE_ARCHIVE ${__WHOLE_ARCHIVE})

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
        __inherited_component_register(${component_target})
    endif()
endmacro()

function(__inherited_component_register component_target)
    __component_get_property(component_lib ${component_target} COMPONENT_LIB)

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

    macro(__component_set_dependencies reqs type)
        foreach(req ${reqs})
            idf_component_get_property(req_lib ${req} COMPONENT_LIB)
            target_link_libraries(${component_lib} ${type} ${req_lib})
        endforeach()
    endmacro()

    if(sources OR __EMBED_FILES OR __EMBED_TXTFILES)
        add_library(${component_lib} STATIC ${sources})
        set_target_properties(${component_lib} PROPERTIES OUTPUT_NAME ${component_name} LINKER_LANGUAGE C)

        idf_build_get_property(include_directories INCLUDE_DIRECTORIES GENERATOR_EXPRESSION)
        target_include_directories(${component_lib} INTERFACE "${include_directories}")

        idf_build_get_property(compile_definitions COMPILE_DEFINITIONS GENERATOR_EXPRESSION)
        target_compile_definitions(${component_lib} PRIVATE "${compile_definitions}")

        idf_build_get_property(compile_options COMPILE_OPTIONS GENERATOR_EXPRESSION)
        foreach(option ${compile_options})
            target_compile_options(${component_lib} PRIVATE ${option})
        endforeach()

        idf_target_include_directories(${component_lib} INTERFACE "${config_dir}")
        idf_target_include_directories(${component_lib} PUBLIC "${__INCLUDE_DIRS}")
        idf_target_include_directories(${component_lib} PRIVATE "${__PRIV_INCLUDE_DIRS}")

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

        idf_target_include_directories(${component_lib} INTERFACE "${config_dir}")
        idf_target_include_directories(${component_lib} INTERFACE "${__INCLUDE_DIRS}")

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

    __component_set_property(${component_target} WHOLE_ARCHIVE ${__WHOLE_ARCHIVE})    # COMPONENT_TARGET is deprecated but is made available with same function
    # as COMPONENT_LIB for compatibility.
    set(COMPONENT_TARGET ${component_lib} PARENT_SCOPE)
    # Make the COMPONENT_LIB variable available in the component CMakeLists.txt
    set(COMPONENT_LIB ${component_lib} PARENT_SCOPE)
endfunction()

#############################################################################
# idf_build()
#############################################################################
function(idf_build)
    if (NOT TIME_T_SIZE)
        include(CheckTypeSize)
        check_type_size("time_t" TIME_T_SIZE)
        message("")
    endif()
    if(TIME_T_SIZE)
        idf_build_set_property(TIME_T_SIZE ${TIME_T_SIZE})
    else()
        message(FATAL_ERROR "Failed to determine sizeof(time_t)")
    endif()

    # build_command.json
    set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

    # attach esp-idf'components compile options
    idf_build_set_property(COMPILE_DEFINITIONS "${IDF_COMPILE_DEFINITIONS}" APPEND)
    idf_build_set_property(COMPILE_OPTIONS "${COMPILE_OPTIONS}" APPEND)
    idf_build_set_property(COMPILE_OPTIONS "${IDF_COMPILE_OPTIONS}" APPEND)
    idf_build_set_property(LINK_OPTIONS "${LINK_OPTIONS}" APPEND)

    message("💡 Resolve dependencies")
    function(__resove_deps)
        # add esp-idf common required components
        idf_build_get_property(common_requires __COMPONENT_REQUIRES_COMMON)
        foreach(iter ${common_requires})
            idf_build_get_property(component_targets __COMPONENT_TARGETS)
            idf_get_component_target(req_target ${iter})

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
                idf_get_component_target(req_target ${iter})
                if (NOT req_target IN_LIST component_targets)
                    idf_component_add(${iter})
                endif()
            else()
                break()
            endif()
        endwhile()
    endfunction()
    __resove_deps()

    message("💡 Kconfig")
    macro(__kconfig)
        # Generate sdkconfig.h/sdkconfig.cmake
        idf_build_get_property(sdkconfig SDKCONFIG)
        idf_build_get_property(sdkconfig_defaults SDKCONFIG_DEFAULTS)
        __kconfig_generate_config("${sdkconfig}" "${sdkconfig_defaults}")

        # Include the sdkconfig cmake file
        idf_build_get_property(sdkconfig_cmake SDKCONFIG_CMAKE)
        include(${sdkconfig_cmake})

        if(CONFIG_COMPILER_OPTIMIZATION_ASSERTIONS_DISABLE)
            add_compile_definitions("NDEBUG")
        endif()

        if(CONFIG_COMPILER_CXX_EXCEPTIONS)
            add_compile_options("$<$<COMPILE_LANGUAGE:CXX>:-fexceptions>")
        else()
            add_compile_options("$<$<COMPILE_LANGUAGE:CXX>:-fno-exceptions>")
        endif()

        if(CONFIG_COMPILER_CXX_RTTI)
            add_compile_options("$<$<COMPILE_LANGUAGE:CXX>:-frtti>")
        else()
            add_compile_options("$<$<COMPILE_LANGUAGE:CXX>:-fno-rtti>")
        endif()

        if(CONFIG_COMPILER_WARN_WRITE_STRINGS)
            add_compile_options("-Wwrite-strings")
        endif()

        if(CONFIG_COMPILER_STACK_CHECK_MODE_NORM)
            add_compile_options("-fstack-protector")
        elseif(CONFIG_COMPILER_STACK_CHECK_MODE_STRONG)
            add_compile_options("-fstack-protector-strong")
        elseif(CONFIG_COMPILER_STACK_CHECK_MODE_ALL)
            add_compile_options("-fstack-protector-all")
        endif()

        if(CONFIG_COMPILER_DUMP_RTL_FILES)
            add_compile_options("-fdump-rtl-expand")
        endif()

        if(CONFIG_COMPILER_HIDE_PATHS_MACROS)
            message("\t❓ CONFIG_COMPILER_HIDE_PATHS_MACROS")
        endif()
        if(CONFIG_APP_REPRODUCIBLE_BUILD)
            message("\t❓ CONFIG_APP_REPRODUCIBLE_BUILD")
        endif()

        if(CONFIG_ESP_SYSTEM_USE_EH_FRAME)
            add_compile_options("-fasynchronous-unwind-tables")
            idf_build_set_property(LINK_OPTIONS "-Wl,--eh-frame-hdr" APPEND)
        endif()

        # SMP FreeRTOS user provided minimal idle hook. This allows the user to provide their own copy of vApplicationMinimalIdleHook()
        if(CONFIG_FREERTOS_USE_MINIMAL_IDLE_HOOK)
            idf_build_set_property(LINK_OPTIONS "-Wl,--wrap=vApplicationMinimalIdleHook" APPEND)
        endif()

        if(CONFIG_COMPILER_SAVE_RESTORE_LIBCALLS)
            idf_build_set_property(COMPILE_OPTIONS "-msave-restore" APPEND)
        endif()
    endmacro()
    __kconfig()

    message("\n💡 Link dependencies")
    idf_build_get_property(component_targets __COMPONENT_TARGETS)

    macro(__import_components)
        # do not remove: some esp-idf'component direct using this value
        set(target ${IDF_TARGET})
        # project_include.cmake
        foreach(component_target ${component_targets})
            __component_get_property(COMPONENT_DIR ${component_target} COMPONENT_DIR)

            if(EXISTS ${COMPONENT_DIR}/project_include.cmake)
                include(${COMPONENT_DIR}/project_include.cmake)
            endif()
        endforeach()

        set(__idf_component_context 1)
        enable_language(C CXX ASM)

        idf_build_get_property(build_dir BUILD_DIR)
        foreach(component_target ${component_targets})
            __component_get_property(prefix ${component_target} __PREFIX)
            __component_get_property(component_name ${component_target} COMPONENT_NAME)
            __component_get_property(COMPONENT_DIR ${component_target} COMPONENT_DIR)

            add_subdirectory(${COMPONENT_DIR} ${build_dir}/${prefix}/${component_name})

            # fix: mbedtls private targets not append compile options
            # if (component_name STREQUAL "mbedtls")
            #     list(APPEND mbedtls_targets "mbedtls" "mbedcrypto" "mbedx509")
            #     foreach(mbedtls_target ${mbedtls_targets})
            #         foreach(option ${IDF_COMPILE_OPTIONS})
            #             target_compile_options(${mbedtls_target} PRIVATE ${option})
            #         endforeach()
            #     endforeach()
            # endif()
        endforeach()
    endmacro()
    __import_components()

    add_executable(${CMAKE_PROJECT_NAME} "dummy.c")
    set_target_properties(${CMAKE_PROJECT_NAME} PROPERTIES OUTPUT_NAME "${CMAKE_PROJECT_NAME}.elf")

    # Propagate link dependencies from component library targets to the executable
    idf_build_get_property(link_depends __LINK_DEPENDS)
    set_target_properties(${CMAKE_PROJECT_NAME} PROPERTIES LINK_DEPENDS "${link_depends}")

    # Set additional link flags for the executable
    idf_build_get_property(link_options LINK_OPTIONS)
    target_link_options(${CMAKE_PROJECT_NAME} PRIVATE "${link_options}")

    if(CMAKE_C_COMPILER_ID STREQUAL "GNU")
        # Add cross-reference table to the map file
        target_link_options(${CMAKE_PROJECT_NAME} PRIVATE "-Wl,--cref")
        # Add this symbol as a hint for idf_size.py to guess the target name
        target_link_options(${CMAKE_PROJECT_NAME} PRIVATE "-Wl,--defsym=IDF_TARGET_${IDF_TARGET}=0")
        # Enable map file output
        target_link_options(${CMAKE_PROJECT_NAME} PRIVATE "-Wl,--Map=${CMAKE_BINARY_DIR}/${CMAKE_PROJECT_NAME}.map")
        unset(idf_target)
    endif()

    if(__PROJECT_GROUP_LINK_COMPONENTS)
        target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE "-Wl,--start-group")
    endif()

    foreach(component_target ${component_targets})
        __component_get_property(whole_archive ${component_target} WHOLE_ARCHIVE)
        __component_get_property(build_component ${component_target} COMPONENT_LIB)
        __component_get_property(component_name ${component_target} COMPONENT_NAME)

        if(whole_archive)
            message(STATUS "Component ${build_component} will be linked with -Wl,--whole-archive")
            target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE
                "-Wl,--whole-archive"
                ${build_component}
                "-Wl,--no-whole-archive")
        else()
            message(STATUS "🔗 Add link library: ${component_name}")
            target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE ${build_component})
        endif()
    endforeach()

    message("\n💡 Add sub-project: bootloader")
    add_subdirectory("bootloader")

    message("")
endfunction()
