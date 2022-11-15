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
        __component_set_property(${COMPONENT_TARGET} KCONFIG "${__KCONFIG}")
        __component_set_property(${COMPONENT_TARGET} KCONFIG_PROJBUILD "${__KCONFIG_PROJBUILD}")

        get_property(depends GLOBAL PROPERTY COMPONENTS_DEPENDS)
        foreach(iter ${__REQUIRES} ${__PRIV_REQUIRES})
            if (NOT iter IN_LIST depends)
                set_property(GLOBAL PROPERTY COMPONENTS_DEPENDS ${iter} APPEND)
            endif()
        endforeach()

        # set( "${__KCONFIG}")
        # set( "${__KCONFIG_PROJBUILD}")

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
        _idf_component_register(${ARGV})
    endif()
endmacro()

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

    set(COMPONENT_ALIAS ${NAMESPACE}::${COMPONENT_NAME}_TEST)
    set(COMPONENT_BUILD_DIR "${CMAKE_BINARY_DIR}/${NAMESPACE}/${COMPONENT_NAME}")

    #   COMPONENT_TARGET
    #   COMPONENT_LIB
    #       .The component target has three underscores as a prefix.
    #       .The corresponding component library only has two.
    set(COMPONENT_TARGET ___${NAMESPACE}_${COMPONENT_NAME}_TEST)
    set(COMPONENT_LIB __${NAMESPACE}_${COMPONENT_NAME}_TEST)

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

    message(STATUS "Add Components: ${COMPONENT_ALIAS}")
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

function(idf_resolve_component_depends)
    while(1)
        get_property(deps GLOBAL PROPERTY COMPONENTS_DEPENDS)
        list(POP_FRONT deps iter)
        set_property(GLOBAL PROPERTY COMPONENTS_DEPENDS ${deps})

        if (iter)
            get_property(components_resolved GLOBAL PROPERTY COMPONENTS_RESOLVED)

            if (NOT iter IN_LIST components_resolved)
                message("Resolve deps: ${iter}")
                idf_component_add("${IDF_PATH}/components/${iter}")
            endif()
        else()
            break()
        endif()
    endwhile()

    message("\n\n")
endfunction()

set(BOOTLOADER_BUILD 1)
idf_component_add("${CMAKE_CURRENT_LIST_DIR}/bootloader")
idf_component_add("${IDF_PATH}/components/bootloader/subproject/components/micro-ecc")
idf_component_add("${CMAKE_CURRENT_LIST_DIR}/esp_app_format")

idf_resolve_component_depends()

__component_add("${CMAKE_CURRENT_LIST_DIR}/bootloader" ${PROJECT_NAME} )
__component_add("${IDF_PATH}/components/bootloader/subproject/components/micro-ecc" ${PROJECT_NAME})
__component_add("${CMAKE_CURRENT_LIST_DIR}/esp_app_format" ${PROJECT_NAME} )

# TODO: -fPIC code?
# idf_build_set_property(C_COMPILE_OPTIONS "-fPIC" APPEND)

IDF_buildroot()
