if ("xtensa" STREQUAL ${IDF_TARGET_ARCH})
    include("${CMAKE_CURRENT_LIST_DIR}/arch/xtensa/.cmake")
elseif("risv" STREQUAL ${IDF_TARGET_ARCH})
    include("${CMAKE_CURRENT_LIST_DIR}/arch/risv/.cmake")
else()
    message(FATAL_ERROR "unknown arch")
endif()

list(APPEND srcs
    "esp_crosscore_int.c"
    "esp_dbg_ubsan.c"                   # debug for undefined behavior, but not sure if its works
    "esp_startup.c"
    "panic.c"
    "panic_handler.c"
    "esp_intr.c"
)

list(APPEND includes
    "include"
    #  NOTE fix: soc => ${IDF_TARGET} subdir using relative paths
    "${IDF_PATH}/components/soc/${IDF_TARGET}/include"
)
list(APPEND priv_includes
)

list(APPEND errata_link_scripts
)

include("${CMAKE_CURRENT_LIST_DIR}/driver/.cmake")
include("${CMAKE_CURRENT_LIST_DIR}/posix/.cmake")

idf_component_register(
    INCLUDE_DIRS
        ${includes}
    PRIV_INCLUDE_DIRS
        ${priv_includes}
    SRCS
        ${srcs}
    PRIV_REQUIRES
        "esp_common"
        "soc"
        # "esp_hw_support"                                # remove later: should merge into driver
    LDFRAGMENTS
        "memap.lf"
)
target_link_libraries(${COMPONENT_LIB} INTERFACE "-u __ubsan_include")

# generate link scripts
idf_build_get_property(sdkconfig_header SDKCONFIG_HEADER)
idf_build_get_property(config_dir CONFIG_DIR)

set(ld_input "${CMAKE_CURRENT_LIST_DIR}/driver/${IDF_TARGET}/ld/memory.ld.in")
set(ld_output "${config_dir}/${IDF_TARGET}_memory.ld")

add_custom_command(
    OUTPUT ${ld_output}
    COMMAND "${CMAKE_C_COMPILER}" -C -P -x c -E -o ${ld_output} -I ${config_dir}
            -I "${CMAKE_CURRENT_LIST_DIR}" ${ld_input}
    DEPENDS ${sdkconfig_header}
    VERBATIM
)
add_custom_target(memory_ld DEPENDS ${ld_output})
# add_dependencies(${COMPONENT_LIB} memory_ld)
target_linker_script(${COMPONENT_LIB} INTERFACE "${ld_output}")

target_linker_script(${COMPONENT_LIB} INTERFACE "driver/${IDF_TARGET}/ld/sections.ld"
    PROCESS "${config_dir}/${IDF_TARGET}_sections.ld"
)

foreach(ld ${errata_link_scripts})
    target_linker_script(${COMPONENT_LIB} INTERFACE "${ld}")
endforeach()
