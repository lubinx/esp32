# idf_build_get_property
#
# @brief Retrieve the value of the specified property related to ESP-IDF build.
#
# @param[out] var the variable to store the value in
# @param[in] property the property to get the value of
#
# @param[in, optional] GENERATOR_EXPRESSION (option) retrieve the generator expression for the property
#                   instead of actual value
function(idf_build_get_property var property)
    cmake_parse_arguments(_ "GENERATOR_EXPRESSION" "" "" ${ARGN})
    if(__GENERATOR_EXPRESSION)
        set(val "$<TARGET_PROPERTY:__idf_build_target,${property}>")
    else()
        get_property(val TARGET __idf_build_target PROPERTY ${property})
    endif()
    set(${var} ${val} PARENT_SCOPE)
endfunction()

# idf_build_set_property
#
# @brief Set the value of the specified property related to ESP-IDF build. The property is
#        also added to the internal list of build properties if it isn't there already.
#
# @param[in] property the property to set the value of
# @param[out] value value of the property
#
# @param[in, optional] APPEND (option) append the value to the current value of the
#                     property instead of replacing it
function(idf_build_set_property property value)
    cmake_parse_arguments(_ "APPEND" "" "" ${ARGN})

    # remove -D
    if(property STREQUAL "COMPILE_DEFINITIONS" AND NOT "${value}" STREQUAL "")
        string(REGEX REPLACE "^-D" "" value "${value}")
    endif()


    if(__APPEND)
        set_property(TARGET __idf_build_target APPEND PROPERTY ${property} ${value})
    else()
        set_property(TARGET __idf_build_target PROPERTY ${property} ${value})
    endif()

    # Keep track of set build properties so that they can be exported to a file that
    # will be included in early expansion script.
    idf_build_get_property(build_properties __BUILD_PROPERTIES)
    if(NOT property IN_LIST build_properties)
        idf_build_set_property(__BUILD_PROPERTIES "${property}" APPEND)
    endif()
endfunction()

# idf_build_unset_property
#
# @brief Unset the value of the specified property related to ESP-IDF build. Equivalent
#        to setting the property to an empty string; though it also removes the property
#        from the internal list of build properties.
#
# @param[in] property the property to unset the value of
function(idf_build_unset_property property)
    idf_build_set_property(${property} "") # set to an empty value
    idf_build_get_property(build_properties __BUILD_PROPERTIES) # remove from tracked properties
    list(REMOVE_ITEM build_properties ${property})
    idf_build_set_property(__BUILD_PROPERTIES "${build_properties}")
endfunction()

#
# Resolve the requirement component to the component target created for that component.
#
function(__build_resolve_and_add_req var component_target req type)
    __component_get_target(_component_target ${req})
    __component_get_property(_component_registered ${component_target} __COMPONENT_REGISTERED)
    if(NOT _component_target OR NOT _component_registered)
        message(FATAL_ERROR "Failed to resolve component '${req}'.")
    endif()
    __component_set_property(${component_target} ${type} ${_component_target} APPEND)
    set(${var} ${_component_target} PARENT_SCOPE)
endfunction()

#
# Write a CMake file containing set build properties, owing to the fact that an internal
# list of properties is maintained in idf_build_set_property() call. This is used to convert
# those set properties to variables in the scope the output file is included in.
#
function(__build_write_properties output_file)
    idf_build_get_property(build_properties __BUILD_PROPERTIES)
    foreach(property ${build_properties})
        idf_build_get_property(val ${property})
        set(build_properties_text "${build_properties_text}\nset(${property} \"${val}\")")
    endforeach()
    file(WRITE ${output_file} "${build_properties_text}")
endfunction()

#
# Prepare for component processing expanding each component's project include
#
macro(__build_process_project_includes)
    # Include the sdkconfig cmake file, since the following operations require
    # knowledge of config values.
    idf_build_get_property(sdkconfig_cmake SDKCONFIG_CMAKE)
    include(${sdkconfig_cmake})

    # Make each build property available as a read-only variable
    idf_build_get_property(build_properties __BUILD_PROPERTIES)
    foreach(build_property ${build_properties})
        idf_build_get_property(val ${build_property})
        set(${build_property} "${val}")
    endforeach()

    # Check that the CMake target value matches the Kconfig target value.
    __target_check()

    idf_build_get_property(build_component_targets __BUILD_COMPONENT_TARGETS)

    # Include each component's project_include.cmake
    foreach(component_target ${build_component_targets})
        __component_get_property(dir ${component_target} COMPONENT_DIR)
        __component_get_property(_name ${component_target} COMPONENT_NAME)
        set(COMPONENT_NAME ${_name})
        set(COMPONENT_DIR ${dir})
        set(COMPONENT_PATH ${dir})  # this is deprecated, users are encouraged to use COMPONENT_DIR;
                                    # retained for compatibility
        if(EXISTS ${COMPONENT_DIR}/project_include.cmake)
            include(${COMPONENT_DIR}/project_include.cmake)
        endif()
    endforeach()
endmacro()

#
# Import configs as build instance properties so that they are accessible
# using idf_build_get_config(). Config has to have been generated before calling
# this command.
#
function(__build_import_configs)
    # Include the sdkconfig cmake file, since the following operations require
    # knowledge of config values.
    idf_build_get_property(sdkconfig_cmake SDKCONFIG_CMAKE)
    include(${sdkconfig_cmake})

    idf_build_set_property(__CONFIG_VARIABLES "${CONFIGS_LIST}")
    foreach(config ${CONFIGS_LIST})
        set_property(TARGET __idf_build_target PROPERTY ${config} "${${config}}")
    endforeach()
endfunction()

# idf_build_process
#
# @brief Main processing step for ESP-IDF build: config generation, adding components to the build,
#        dependency resolution, etc.
#
# @param[in] target ESP-IDF target
#
# @param[in, optional] PROJECT_DIR (single value) directory of the main project the buildsystem
#                      is processed for; defaults to CMAKE_SOURCE_DIR
# @param[in, optional] PROJECT_VER (single value) version string of the main project; defaults
#                      to 1
# @param[in, optional] PROJECT_NAME (single value) main project name, defaults to CMAKE_PROJECT_NAME
# @param[in, optional] SDKCONFIG (single value) sdkconfig output path, defaults to PROJECT_DIR/sdkconfig
#                       if PROJECT_DIR is set and CMAKE_SOURCE_DIR/sdkconfig if not
# @param[in, optional] SDKCONFIG_DEFAULTS (single value) config defaults file to use for the build; defaults
#                       to none (Kconfig defaults or previously generated config are used)
# @param[in, optional] BUILD_DIR (single value) directory for build artifacts; defautls to CMAKE_BINARY_DIR
# @param[in, optional] COMPONENTS (multivalue) select components to process among the components
#                       known by the build system
#                       (added via `idf_build_component`). This argument is used to trim the build.
#                       Other components are automatically added if they are required
#                       in the dependency chain, i.e.
#                       the public and private requirements of the components in this list
#                       are automatically added, and in
#                       turn the public and private requirements of those requirements,
#                       so on and so forth. If not specified, all components known to the build system
#                       are processed.
macro(idf_build_process)
    set(options)
    set(single_value PROJECT_DIR PROJECT_VER PROJECT_NAME BUILD_DIR SDKCONFIG)
    set(multi_value COMPONENTS SDKCONFIG_DEFAULTS)
    cmake_parse_arguments(_ "${options}" "${single_value}" "${multi_value}" ${ARGN})

    __build_set_default(PROJECT_DIR ${CMAKE_SOURCE_DIR})
    __build_set_default(PROJECT_NAME ${CMAKE_PROJECT_NAME})
    __build_set_default(PROJECT_VER 1)
    __build_set_default(BUILD_DIR ${CMAKE_BINARY_DIR})

    idf_build_get_property(project_dir PROJECT_DIR)
    __build_set_default(SDKCONFIG "${project_dir}/sdkconfig")

    __build_set_default(SDKCONFIG_DEFAULTS "")

    idf_build_get_property(arch IDF_TARGET_ARCH)

    if(NOT "${IDF_TARGET}" STREQUAL "linux")
        idf_build_set_property(__COMPONENT_REQUIRES_COMMON ${arch} APPEND)
    endif()

    # Call for component manager to download dependencies for all components
    idf_build_get_property(idf_component_manager IDF_COMPONENT_MANAGER)
    if(idf_component_manager EQUAL 1)
        idf_build_get_property(build_dir BUILD_DIR)
        set(managed_components_list_file ${build_dir}/managed_components_list.temp.cmake)
        set(local_components_list_file ${build_dir}/local_components_list.temp.yml)

        set(__contents "components:\n")
        foreach(__component_name ${components})
            idf_component_get_property(__component_dir ${__component_name} COMPONENT_DIR)
            set(__contents "${__contents}  - name: \"${__component_name}\"\n    path: \"${__component_dir}\"\n")
        endforeach()

        file(WRITE ${local_components_list_file} "${__contents}")

        # Call for the component manager to prepare remote dependencies
        idf_build_get_property(python PYTHON)
        idf_build_get_property(component_manager_interface_version __COMPONENT_MANAGER_INTERFACE_VERSION)
        execute_process(COMMAND ${python}
            "-m"
            "idf_component_manager.prepare_components"
            "--project_dir=${project_dir}"
            "--interface_version=${component_manager_interface_version}"
            "prepare_dependencies"
            "--local_components_list_file=${local_components_list_file}"
            "--managed_components_list_file=${managed_components_list_file}"
            RESULT_VARIABLE result
            ERROR_VARIABLE error)

        if(NOT result EQUAL 0)
            message(FATAL_ERROR "${error}")
        endif()

        include(${managed_components_list_file})

        # Add managed components to list of all components
        # `managed_components` contains the list of components installed by the component manager
        # It is defined in the temporary managed_components_list_file file
        set(__COMPONENTS "${__COMPONENTS};${managed_components}")

        file(REMOVE ${managed_components_list_file})
        file(REMOVE ${local_components_list_file})
    else()
        message(VERBOSE "IDF Component manager was explicitly disabled by setting IDF_COMPONENT_MANAGER=0")

        idf_build_get_property(__component_targets __COMPONENT_TARGETS)
        set(__components_with_manifests "")
        foreach(__component_target ${__component_targets})
            __component_get_property(__component_dir ${__component_target} COMPONENT_DIR)
            if(EXISTS "${__component_dir}/idf_component.yml")
                set(__components_with_manifests "${__components_with_manifests}\t${__component_dir}\n")
            endif()
        endforeach()

        if(NOT "${__components_with_manifests}" STREQUAL "")
            message(WARNING "\"idf_component.yml\" file was found for components:\n${__components_with_manifests}"
                    "However, the component manager is not enabled.")
        endif()
    endif()

    # Perform early expansion of component CMakeLists.txt in CMake scripting mode.
    # It is here we retrieve the public and private requirements of each component.
    # It is also here we add the common component requirements to each component's
    # own requirements.
    __component_get_requirements()

    idf_build_get_property(component_targets __COMPONENT_TARGETS)

    # Finally, do component expansion. In this case it simply means getting a final list
    # of build component targets given the requirements set by each component.

    # Check if we need to trim the components first, and build initial components list
    # from that.
    if(__COMPONENTS)
        unset(component_targets)
        foreach(component ${__COMPONENTS})
            __component_get_target(component_target ${component})
            if(NOT component_target)
                message(FATAL_ERROR "Failed to resolve component '${component}'.")
            endif()
            list(APPEND component_targets ${component_target})
        endforeach()
    endif()

    foreach(component_target ${component_targets})
        __build_expand_requirements(${component_target})
    endforeach()
    idf_build_unset_property(__COMPONENT_TARGETS_SEEN)

    # Get a list of common component requirements in component targets form (previously
    # we just have a list of component names)
    idf_build_get_property(common_reqs __COMPONENT_REQUIRES_COMMON)
    foreach(common_req ${common_reqs})
        __component_get_target(component_target ${common_req})
        __component_get_property(lib ${component_target} COMPONENT_LIB)
        idf_build_set_property(___COMPONENT_REQUIRES_COMMON ${lib} APPEND)
    endforeach()

    # Generate config values in different formats
    idf_build_get_property(sdkconfig SDKCONFIG)
    idf_build_get_property(sdkconfig_defaults SDKCONFIG_DEFAULTS)
    __kconfig_generate_config("${sdkconfig}" "${sdkconfig_defaults}")
    __build_import_configs()

    # All targets built under this scope is with the ESP-IDF build system
    set(ESP_PLATFORM 1)
    idf_build_set_property(COMPILE_DEFINITIONS "ESP_PLATFORM" APPEND)

    # Perform component processing (inclusion of project_include.cmake, adding component
    # subdirectories, creating library targets, linking libraries, etc.)
    set(target ${IDF_TARGET})
    __build_process_project_includes()

    idf_build_get_property(idf_path IDF_PATH)
    add_subdirectory(${idf_path} ${build_dir}/esp-idf)

    unset(ESP_PLATFORM)
endmacro()

# idf_build_executable
#
# @brief Specify the executable the build system can attach dependencies to (for generating
# files used for linking, targets which should execute before creating the specified executable,
# generating additional binary files, generating files related to flashing, etc.)
function(idf_build_executable elf)
    # Set additional link flags for the executable
    idf_build_get_property(link_options LINK_OPTIONS)
    set_property(TARGET ${elf} APPEND PROPERTY LINK_OPTIONS "${link_options}")

    # Propagate link dependencies from component library targets to the executable
    idf_build_get_property(link_depends __LINK_DEPENDS)
    set_property(TARGET ${elf} APPEND PROPERTY LINK_DEPENDS "${link_depends}")

    # Set the EXECUTABLE_NAME and EXECUTABLE properties since there are generator expression
    # from components that depend on it
    get_filename_component(elf_name ${elf} NAME_WE)
    get_target_property(elf_dir ${elf} BINARY_DIR)

    idf_build_set_property(EXECUTABLE_NAME ${elf_name})
    idf_build_set_property(EXECUTABLE ${elf})
    idf_build_set_property(EXECUTABLE_DIR "${elf_dir}")

    # Add dependency of the build target to the executable
    add_dependencies(${elf} __idf_build_target)
endfunction()

# idf_build_get_config
#
# @brief Get value of specified config variable
function(idf_build_get_config var config)
    cmake_parse_arguments(_ "GENERATOR_EXPRESSION" "" "" ${ARGN})
    if(__GENERATOR_EXPRESSION)
        set(val "$<TARGET_PROPERTY:__idf_build_target,${config}>")
    else()
        get_property(val TARGET __idf_build_target PROPERTY ${config})
    endif()
    set(${var} ${val} PARENT_SCOPE)
endfunction()
