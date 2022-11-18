function(__build_resolve_and_add_req var component_target req type)
    __component_get_target(_component_target ${req})

    __component_set_property(${component_target} ${type} ${_component_target} APPEND)
    set(${var} ${_component_target} PARENT_SCOPE)
    # message("## component_target: ${component_target} type: ${type} wtf: ${_component_target}")
endfunction()

function(__build_expand_requirements component_target)
    # Since there are circular dependencies, make sure that we do not infinitely
    # expand requirements for each component.
    idf_build_get_property(component_targets_seen __COMPONENT_TARGETS_SEEN)
    if(component_target IN_LIST component_targets_seen)
        return()
    endif()

    idf_build_set_property(__COMPONENT_TARGETS_SEEN ${component_target} APPEND)

    __component_get_property(component_name ${component_target} COMPONENT_NAME)
    __component_get_property(reqs ${component_target} REQUIRES)
    __component_get_property(priv_reqs ${component_target} PRIV_REQUIRES)

    idf_build_get_property(common_reqs __COMPONENT_REQUIRES_COMMON)
    list(APPEND reqs ${common_reqs})

    if(reqs)
        list(REMOVE_DUPLICATES reqs)
        list(REMOVE_ITEM reqs ${component_name})
    endif()

    foreach(req ${reqs})
        __build_resolve_and_add_req(_component_target ${component_target} ${req} __REQUIRES)
        __build_expand_requirements(${_component_target})
    endforeach()

    foreach(req ${priv_reqs})
        __build_resolve_and_add_req(_component_target ${component_target} ${req} __PRIV_REQUIRES)
        __build_expand_requirements(${_component_target})
    endforeach()
endfunction()
