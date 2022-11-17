function(__build_resolve_and_add_req var component_target req type)
    __component_get_target(_component_target ${req})
    __component_get_property(_component_registered ${component_target} __COMPONENT_REGISTERED)
    if(NOT _component_target OR NOT _component_registered)
        message(FATAL_ERROR "Failed to resolve component '${req}'.")
    endif()
    __component_set_property(${component_target} ${type} ${_component_target} APPEND)
    set(${var} ${_component_target} PARENT_SCOPE)

    message("## component_target: ${component_target} type: ${type} wtf: ${_component_target}")
endfunction()

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
