#
# Internal function for retrieving component properties from a component target.
#
function(__component_get_property var component_target property)
    get_property(val TARGET ${component_target} PROPERTY ${property})
    set(${var} "${val}" PARENT_SCOPE)
endfunction()

#
# Internal function for setting component properties on a component target. As with build properties,
# set properties are also keeped track of.
#
function(__component_set_property component_target property val)
    cmake_parse_arguments(_ "APPEND" "" "" ${ARGN})

    if(__APPEND)
        set_property(TARGET ${component_target} APPEND PROPERTY ${property} "${val}")
    else()
        set_property(TARGET ${component_target} PROPERTY ${property} "${val}")
    endif()
endfunction()

#
# Given a component name or alias, get the corresponding component target.
#
function(__component_get_target var name_or_alias)
    # Look at previously resolved names or aliases
    idf_build_get_property(component_names_resolved __COMPONENT_NAMES_RESOLVED)
    list(FIND component_names_resolved ${name_or_alias} result)
    if(NOT result EQUAL -1)
        # If it has been resolved before, return that value. The index is the same
        # as in __COMPONENT_NAMES_RESOLVED as these are parallel lists.
        idf_build_get_property(component_targets_resolved __COMPONENT_TARGETS_RESOLVED)
        list(GET component_targets_resolved ${result} target)
        set(${var} ${target} PARENT_SCOPE)
        return()
    endif()

    idf_build_get_property(component_targets __COMPONENT_TARGETS)

    # Assume first that the paramters is an alias.
    string(REPLACE "::" "_" name_or_alias "${name_or_alias}")
    set(component_target ___${name_or_alias})

    if(component_target IN_LIST component_targets)
        set(${var} ${component_target} PARENT_SCOPE)
        set(target ${component_target})
    else() # assumption is wrong, try to look for it manually
        unset(target)
        foreach(component_target ${component_targets})
            __component_get_property(_component_name ${component_target} COMPONENT_NAME)
            if(name_or_alias STREQUAL _component_name)
                set(target ${component_target})
                break()
            endif()
        endforeach()
        set(${var} ${target} PARENT_SCOPE)
    endif()

    # Save the resolved name or alias
    if(target)
        idf_build_set_property(__COMPONENT_NAMES_RESOLVED ${name_or_alias} APPEND)
        idf_build_set_property(__COMPONENT_TARGETS_RESOLVED ${target} APPEND)
    endif()
endfunction()

# __component_add_sources, __component_check_target, __component_add_include_dirs
#
# Utility macros for component registration. Adds source files and checks target requirements,
# and adds include directories respectively.
macro(__component_add_sources sources)
    set(sources "")
    if(__SRCS)
        if(__SRC_DIRS)
            message(WARNING "SRCS and SRC_DIRS are both specified; ignoring SRC_DIRS.")
        endif()
        foreach(src ${__SRCS})
            get_filename_component(src "${src}" ABSOLUTE BASE_DIR ${COMPONENT_DIR})
            list(APPEND sources ${src})
        endforeach()
    else()
        if(__SRC_DIRS)
            foreach(dir ${__SRC_DIRS})
                get_filename_component(abs_dir ${dir} ABSOLUTE BASE_DIR ${COMPONENT_DIR})

                if(NOT IS_DIRECTORY ${abs_dir})
                    message(FATAL_ERROR "SRC_DIRS entry '${dir}' does not exist.")
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
    endif()

    list(REMOVE_DUPLICATES sources)
endmacro()

macro(__component_add_include_dirs lib dirs type)
    foreach(dir ${dirs})
        get_filename_component(_dir ${dir} ABSOLUTE BASE_DIR ${CMAKE_CURRENT_LIST_DIR})
        if(NOT IS_DIRECTORY ${_dir})
            message(FATAL_ERROR "Include directory '${_dir}' is not a directory.")
        endif()
        target_include_directories(${lib} ${type} ${_dir})
    endforeach()
endmacro()

# __component_set_dependencies, __component_set_all_dependencies
#
#  Links public and private requirements for the currently processed component
macro(__component_set_dependencies reqs type)
    foreach(req ${reqs})
        if(req IN_LIST build_component_targets)
            __component_get_property(req_lib ${req} COMPONENT_LIB)
            if("${type}" STREQUAL "PRIVATE")
                set_property(TARGET ${component_lib} APPEND PROPERTY LINK_LIBRARIES ${req_lib})
                set_property(TARGET ${component_lib} APPEND PROPERTY INTERFACE_LINK_LIBRARIES $<LINK_ONLY:${req_lib}>)
            elseif("${type}" STREQUAL "PUBLIC")
                set_property(TARGET ${component_lib} APPEND PROPERTY LINK_LIBRARIES ${req_lib})
                set_property(TARGET ${component_lib} APPEND PROPERTY INTERFACE_LINK_LIBRARIES ${req_lib})
            else() # INTERFACE
                set_property(TARGET ${component_lib} APPEND PROPERTY INTERFACE_LINK_LIBRARIES ${req_lib})
            endif()
        endif()
    endforeach()
endmacro()

macro(__component_set_all_dependencies)
    __component_get_property(type ${component_target} COMPONENT_TYPE)
    idf_build_get_property(build_component_targets __BUILD_COMPONENT_TARGETS)

    if(NOT type STREQUAL CONFIG_ONLY)
        __component_get_property(reqs ${component_target} __REQUIRES)
        __component_set_dependencies("${reqs}" PUBLIC)

        __component_get_property(priv_reqs ${component_target} __PRIV_REQUIRES)
        __component_set_dependencies("${priv_reqs}" PRIVATE)
    else()
        __component_get_property(reqs ${component_target} __REQUIRES)
        __component_set_dependencies("${reqs}" INTERFACE)
    endif()
endmacro()


# idf_component_get_property
#
# @brief Retrieve the value of the specified component property
#
# @param[out] var the variable to store the value of the property in
# @param[in] component the component name or alias to get the value of the property of
# @param[in] property the property to get the value of
#
# @param[in, optional] GENERATOR_EXPRESSION (option) retrieve the generator expression for the property
#                   instead of actual value
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

# idf_component_set_property
#
# @brief Set the value of the specified component property related. The property is
#        also added to the internal list of component properties if it isn't there already.
#
# @param[in] component component name or alias of the component to set the property of
# @param[in] property the property to set the value of
# @param[out] value value of the property to set to
#
# @param[in, optional] APPEND (option) append the value to the current value of the
#                     property instead of replacing it
function(idf_component_set_property component property val)
    cmake_parse_arguments(_ "APPEND" "" "" ${ARGN})
    __component_get_target(component_target ${component})

    if(__APPEND)
        __component_set_property(${component_target} ${property} "${val}" APPEND)
    else()
        __component_set_property(${component_target} ${property} "${val}")
    endif()
endfunction()

# idf_component_optional_requires
#
# @brief Add a dependency on a given component only if it is included in the build.
#
# Calling idf_component_optional_requires(PRIVATE dependency_name) has the similar effect to
# target_link_libraries(${COMPONENT_LIB} PRIVATE idf::dependency_name), only if 'dependency_name'
# component is part of the build. Otherwise, no dependency gets added. Multiple names may be given.
#
# @param[in]  type of the dependency, one of: PRIVATE, PUBLIC, INTERFACE
# @param[in, multivalue] list of component names which should be added as dependencies
#
function(idf_component_optional_requires req_type)
    set(optional_reqs ${ARGN})
    idf_build_get_property(build_components BUILD_COMPONENTS)

    foreach(req ${optional_reqs})
        if(req IN_LIST build_components)
            idf_component_get_property(req_lib ${req} COMPONENT_LIB)
            target_link_libraries(${COMPONENT_LIB} ${req_type} ${req_lib})
        endif()
    endforeach()
endfunction()
