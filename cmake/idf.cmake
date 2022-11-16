#############################################################################
# 💡 environment variables
#############################################################################
if (NOT IDF_TARGET)
    set(IDF_TARGET "esp32s3" CACHE STRING "esp-idf build target")
    message(STATUS "❓ variable IDF_TARGET is not set, default set to esp32s3")
endif()
set_property(CACHE IDF_TARGET PROPERTY STRINGS esp32 esp32s2 esp32s3 esp32c3 esp32h2 esp32c2 esp32c6)

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
    ""
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
include(${IDF_CMAKE_PATH}/component.cmake)
include(${IDF_CMAKE_PATH}/new_build.cmake)

#############################################################################
# overrides
#############################################################################
function(check_expected_tool_version tool_name tool_path)
    # required by ${IDF_PATH}/components/<dir>/project_include.cmake
    #   esp_common/project_include.cmake is only SEEN using this function
endfunction()

function(depgraph_add_edge)
endfunction()

function(__target_check)
endfunction()

macro(__build_set_default)
endmacro()

macro(__build_set_default_build_specifications)
endmacro()

macro(__build_set_lang_version)
endmacro()

#############################################################################
# esp-idf git submodules
#############################################################################
function (git_submodule_check_once)
    # get git submodules from ${IDF_PATH} if submodules was not initialized
    if (NOT GIT_SUBMODULES_CHECKED)
    set(GIT_SUBMODULES_CHECKED true CACHE BOOL "esp-idf git submodule checked")
        message(STATUS "💡 Checking ESP-IDF components submodules, this will only execute once")

        include(${IDF_CMAKE_PATH}/git_submodules.cmake)
        git_submodule_check(${IDF_PATH})

        message("")
    endif()
endfunction()
git_submodule_check_once()

#############################################################################
# build.cmake
#############################################################################
function(__build_init)
    # setup for idf_build_set_property() / idf_build_get_property()
    add_library(__idf_build_target STATIC IMPORTED GLOBAL)

    # __build_set_default_build_specifications
    idf_build_set_property(COMPILE_DEFINITIONS "${COMPILE_DEFINITIONS}")
    idf_build_set_property(COMPILE_OPTIONS "${COMPILE_OPTIONS}")
    idf_build_set_property(C_COMPILE_OPTIONS "${C_COMPILE_OPTIONS}")
    idf_build_set_property(CXX_COMPILE_OPTIONS "${CXX_COMPILE_OPTIONS}")

    idf_build_set_property(IDF_TARGET ${IDF_TARGET})
    idf_build_set_property(IDF_TARGET_ARCH ${IDF_TARGET_ARCH})
    idf_build_set_property(IDF_PATH ${IDF_PATH})
    idf_build_set_property(IDF_CMAKE_PATH ${IDF_CMAKE_PATH})
    # python
    idf_build_set_property(PYTHON "${PYTHON_ENV}")

    idf_build_set_property(PROJECT_DIR ${CMAKE_SOURCE_DIR})
    idf_build_set_property(BUILD_DIR ${CMAKE_BINARY_DIR})

    idf_build_set_property(SDKCONFIG "${CMAKE_SOURCE_DIR}/sdkconfig") # from PROJECT_DIR
    idf_build_set_property(SDKCONFIG_DEFAULTS "")

    # __build_get_idf_git_revision()
    idf_build_set_property(COMPILE_DEFINITIONS "IDF_VER=\"$ENV{IDF_VERSION}\"")

    # build esp-idf components
    idf_build_set_property(__PREFIX esp-idf)

    __build_set_lang_version()
    __kconfig_init()

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

    message(STATUS "ESP-IDF environment initialized")
    message("\tSource path: ${IDF_PATH}")
    message("\tTools path: ${IDF_ENV_PATH}")
    message("\tBuilding target: ${IDF_TARGET}")
    message("")
endfunction()

# 💡 build initialization
__build_init()
#############################################################################

function(idf_component_add COMPONENT_DIR) # NOTE: *override* optional: NAMESPACE
    # NAMESPACE
    list(POP_FRONT ARGN NAMESPACE)

    get_filename_component(COMPONENT_NAME ${COMPONENT_DIR} NAME)
    get_filename_component(PARENT_DIR ${COMPONENT_DIR} DIRECTORY)

    if (${PARENT_DIR} STREQUAL "${IDF_PATH}/components")
        if (NOT NAMESPACE)
            idf_build_get_property(NAMESPACE __PREFIX)
        endif()
    else()
        if (NOT NAMESPACE)
            set(NAMESPACE ${PROJECT_NAME})
        endif()
    endif()

    set(COMPONENT_ALIAS ${NAMESPACE}::${COMPONENT_NAME})
    set(COMPONENT_BUILD_DIR "${CMAKE_BINARY_DIR}/${NAMESPACE}/${COMPONENT_NAME}")

    #   COMPONENT_TARGET
    #   COMPONENT_LIB
    #       .The component target has three underscores as a prefix.
    #       .The corresponding component library only has two.
    set(COMPONENT_TARGET ___${NAMESPACE}_${COMPONENT_NAME})
    set(COMPONENT_LIB __${NAMESPACE}_${COMPONENT_NAME})

    get_property(components_resolved GLOBAL PROPERTY COMPONENTS_RESOLVED)
    if(NOT COMPONENT_NAME IN_LIST components_resolved)
        if(NOT EXISTS "${COMPONENT_DIR}/CMakeLists.txt")
            message(FATAL_ERROR "Directory '${COMPONENT_DIR}' does not contain a component.")
        endif()
        set_property(GLOBAL PROPERTY COMPONENTS_RESOLVED ${COMPONENT_NAME} APPEND)

        # TODO: remove this
        idf_build_set_property(__COMPONENT_TARGETS ${COMPONENT_TARGET} APPEND)
        idf_build_set_property(__BUILD_COMPONENT_TARGETS ${COMPONENT_TARGET} APPEND)
        idf_build_set_property(BUILD_COMPONENT_ALIASES ${COMPONENT_ALIAS} APPEND)
    else()
        message(WARNING "Components ${COMPONENT_NAME} was already added.")
        return()
    endif()

    # TODO: sub components
    # if (EXISTS "${COMPONENT_DIR}/components")
    # endif()
    add_library(${COMPONENT_TARGET} STATIC IMPORTED)

    # Set the basic properties of the component
    __component_set_property(${COMPONENT_TARGET} __PREFIX ${NAMESPACE})
    __component_set_property(${COMPONENT_TARGET} COMPONENT_NAME ${COMPONENT_NAME})
    __component_set_property(${COMPONENT_TARGET} COMPONENT_ALIAS ${COMPONENT_ALIAS})
    __component_set_property(${COMPONENT_TARGET} COMPONENT_LIB ${COMPONENT_LIB})
    __component_set_property(${COMPONENT_TARGET} COMPONENT_DIR ${COMPONENT_DIR})
    # build dir
    __component_set_property(${COMPONENT_TARGET} COMPONENT_BUILD_DIR ${COMPONENT_BUILD_DIR})

    # Set Kconfig related properties on the component
    __kconfig_component_init(${COMPONENT_TARGET})
    # set BUILD_COMPONENT_DIRS build property
    idf_build_set_property(BUILD_COMPONENT_DIRS ${COMPONENT_DIR} APPEND)

    # ⚓ call macro idf_component_register()
    include(${COMPONENT_DIR}/CMakeLists.txt)
    # ⚓ unreachable here, returned by macro expansion
endfunction()

macro(idf_component_register)
    if(NOT __idf_component_context)
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

        if(__REQUIRED_IDF_TARGETS)
            if(NOT IDF_TARGET IN_LIST __REQUIRED_IDF_TARGETS)
                message(FATAL_ERROR "Component ${COMPONENT_NAME} only supports targets: ${__REQUIRED_IDF_TARGETS}")
            endif()
        endif()

        __component_set_property(${COMPONENT_TARGET} REQUIRES "${__REQUIRES}")
        __component_set_property(${COMPONENT_TARGET} PRIV_REQUIRES "${__PRIV_REQUIRES}")

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
        # __inherited_component_register()
        _idf_component_register(${ARGV})
    endif()
endmacro()

function(__inherited_component_register)
    __component_check_target()
    __component_add_sources(sources)

    # Add component manifest to the list of dependencies
    set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${COMPONENT_DIR}/idf_component.yml")

    # Create the final target for the component. This target is the target that is
    # visible outside the build system.
    __component_get_target(component_target ${COMPONENT_ALIAS})
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

    if(common_reqs) # check whether common_reqs exists, this may be the case in minimalistic host unit test builds
        list(REMOVE_ITEM common_reqs ${component_lib})
    endif()
    link_libraries(${common_reqs})

    idf_build_get_property(config_dir CONFIG_DIR)

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

    # Make the COMPONENT_LIB variable available in the component CMakeLists.txt
    set(COMPONENT_LIB ${component_lib} PARENT_SCOPE)
    # COMPONENT_TARGET is deprecated but is made available with same function
    # as COMPONENT_LIB for compatibility.
    set(COMPONENT_TARGET ${component_lib} PARENT_SCOPE)

    __component_set_properties()
endfunction()

function(idf_resolve_component_depends)
    get_property(components_resolved GLOBAL PROPERTY COMPONENTS_RESOLVED)

    if (NOT IDF_TARGET_ARCH STREQUAL "")
        if (NOT IDF_TARGET_ARCH IN_LIST components_resolved)
            message("Resolve chip ${IDF_TARGET_ARCH} architecture")
            idf_component_add("${IDF_PATH}/components/${IDF_TARGET_ARCH}")
        endif()
    endif()

    message("Resolve ESP-IDF required components")

    if (NOT "freertos" IN_LIST components_resolved)
        idf_component_add("${IDF_PATH}/components/freertos")
    endif()
    if (NOT "log" IN_LIST components_resolved)
        idf_component_add("${IDF_PATH}/components/log")
    endif()
    if (NOT "esptool_py" IN_LIST components_resolved)
        idf_component_add("${IDF_PATH}/components/esptool_py")
    endif()
    if (NOT "esp_common" IN_LIST components_resolved)
        idf_component_add("${IDF_PATH}/components/esp_common")
    endif()

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
                message("Resolve dependency: ${iter}")
                idf_component_add("${IDF_PATH}/components/${iter}")
            endif()
        else()
            break()
        endif()
    endwhile()
endfunction()

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
