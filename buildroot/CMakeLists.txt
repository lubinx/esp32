# The following lines of boilerplate have to be in your project's
# CMakeLists in this exact order for cmake to work correctly
cmake_minimum_required(VERSION 3.5)

set(SDKCONFIG "${CMAKE_CURRENT_LIST_DIR}/sdkconfig")
include(idf.cmake)

project(buildroot)

macro(idf_component_register)
    # 0 == __idf_component_context
    #   will calling from include(<component dir>) idf_component_register()
    # other size
    #   add_subdirectory
    if(NOT __idf_component_context)
        set(options
            WHOLE_ARCHIVE
        )
        set(single_value
            KCONFIG KCONFIG_PROJBUILD
        )
        set(multi_value SRCS SRC_DIRS EXCLUDE_SRCS
            INCLUDE_DIRS PRIV_INCLUDE_DIRS LDFRAGMENTS REQUIRES
            PRIV_REQUIRES REQUIRED_IDF_TARGETS EMBED_FILES EMBED_TXTFILES
        )
        cmake_parse_arguments(_ "${options}" "${single_value}" "${multi_value}" ${ARGN})

        __component_set_property(${COMPONENT_TARGET} PRIV_REQUIRES "${__PRIV_REQUIRES}")
        __component_set_property(${COMPONENT_TARGET} REQUIRES "${__REQUIRES}")
        __component_set_property(${COMPONENT_TARGET} KCONFIG "${__KCONFIG}")
        __component_set_property(${COMPONENT_TARGET} KCONFIG_PROJBUILD "${__KCONFIG_PROJBUILD}")

        message("=== ${NAMESPACE}::${COMPONENT_NAME}")
        message("\tcomponent dir: ${COMPONENT_DIR}")
        message("\tbuild dir: ${COMPONENT_BUILD_DIR}")

        # tick is here, macro will cause caller to return at here
        return()
    else()
        _idf_component_register(${ARGV})
    endif()
endmacro()

function(target_linker_script target deptype scriptfiles)
    if(__idf_component_context)
        _target_linker_script(${ARGV})
    endif()
endfunction()


function(add_component COMPONENT_NAME) # NOTE: *override* add optional args: NAMESPACE COMPONENT_DIR
    # NAMESPACE
    list(POP_FRONT ARGN NAMESPACE)
    if (NOT NAMESPACE)
        idf_build_get_property(NAMESPACE __PREFIX)
    endif()

    if (${COMPONENT_NAME} MATCHES "${IDF_PATH}/components*")
        set(COMPONENT_DIR ${COMPONENT_NAME})
        get_filename_component(COMPONENT_NAME ${COMPONENT_DIR} NAME)
    else()
        list(POP_FRONT ARGN COMPONENT_DIR)

        if (NOT COMPONENT_DIR)
            set(COMPONENT_DIR "${IDF_PATH}/components/${COMPONENT_NAME}")
        else()
            set(COMPONENT_DIR "${COMPONENT_DIR}/${COMPONENT_NAME}")
        endif()
    endif()
    if(NOT EXISTS "${COMPONENT_DIR}/CMakeLists.txt")
        message(FATAL_ERROR "Directory '${COMPONENT_DIR}' does not contain a component.")
    endif()
    set(COMPONENT_BUILD_DIR "${CMAKE_BINARY_DIR}/${NAMESPACE}/${COMPONENT_NAME}")

    # 💡
    #   COMPONENT_NAME
    #   COMPONENT_DIR / COMPONENT_BUILD_DIR
    #
    #   COMPONENT_ALIAS
    #   COMPONENT_TARGET
    #   COMPONENT_LIB
    #       .The component target has three underscores as a prefix.
    #       .The corresponding component library only has two.
    #
    set(COMPONENT_ALIAS ${NAMESPACE}::${COMPONENT_NAME}_TEST)
    set(COMPONENT_TARGET ___${NAMESPACE}_${COMPONENT_NAME}_TEST)
    set(COMPONENT_LIB __${NAMESPACE}_${COMPONENT_NAME}_TEST)

    get_property(components_resolved GLOBAL PROPERTY RESOLVED_COMPONENTS)
    if(COMPONENT_TARGET IN_LIST components_resolved)
        message(WARNING "Components ${COMPONENT_NAME} was already added.")
        return()
    endif()

    message(STATUS "dynamic Add Components: ${COMPONENT_ALIAS}")
    set_property(GLOBAL PROPERTY RESOLVED_COMPONENTS ${COMPONENT_TARGET} APPEND)

    add_library(${COMPONENT_TARGET} STATIC IMPORTED)

    # Set the basic properties of the component
    __component_set_property(${COMPONENT_TARGET} __PREFIX ${NAMESPACE})
    __component_set_property(${COMPONENT_TARGET} COMPONENT_NAME ${COMPONENT_NAME})
    __component_set_property(${COMPONENT_TARGET} COMPONENT_ALIAS ${COMPONENT_ALIAS})
    __component_set_property(${COMPONENT_TARGET} COMPONENT_LIB ${COMPONENT_LIB})
    __component_set_property(${COMPONENT_TARGET} COMPONENT_DIR ${COMPONENT_DIR})
    # build dir
    __component_set_property(${COMPONENT_TARGET} COMPONENT_BUILD_DIR ${COMPONENT_BUILD_DIR})

    # add_subdirectory(${COMPONENT_DIR} ${COMPONENT_BUILD_DIR})

    # Set Kconfig related properties on the component
    # __kconfig_component_init(${COMPONENT_TARGET})

    # set BUILD_COMPONENT_DIRS build property
    # idf_build_set_property(BUILD_COMPONENT_DIRS ${COMPONENT_DIR} APPEND)

    # set(__idf_component_context 1)
    # include(${COMPONENT_DIR} OPTIONAL)
    # set(__idf_component_context 0)

    # __component_get_property(reqs ${COMPONENT_TARGET} __REQUIRES)
    # __component_get_property(priv_reqs ${COMPONENT_TARGET} __PRIV_REQUIRES)

    # foreach(iter ${priv_reqs})
    #     LIST(APPEND reqs ${iter})
    # endforeach()
endfunction()

# add_component("bootloader" ${PROJECT_NAME} ${CMAKE_CURRENT_LIST_DIR})
# add_component("bootloader_support")
# return()

idf_component_add("bootloader" ${PROJECT_NAME} ${CMAKE_CURRENT_LIST_DIR})
idf_component_add("micro-ecc" ${PROJECT_NAME} "${IDF_PATH}/components/bootloader/subproject/components")

# A number of these components are implemented as config-only when built in the bootloader
set(COMPONENTS
    # bootloader
    # micro-ecc
    bootloader_support
    esptool_py
    esp_hw_support
    esp_system
    freertos
    hal
    # partition_table
    soc
    log
    spi_flash
    efuse
    esp_system
    newlib
)

set(common_req log esp_rom esp_common esp_hw_support newlib)
idf_build_set_property(__COMPONENT_REQUIRES_COMMON "${common_req}")

# TODO: -fPIC code?
# idf_build_set_property(C_COMPILE_OPTIONS "-fPIC" APPEND)

IDF_buildroot()
