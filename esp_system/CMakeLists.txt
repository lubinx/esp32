if ("xtensa" STREQUAL ${IDF_TARGET_ARCH})
    include("${CMAKE_CURRENT_LIST_DIR}/arch/xtensa/.cmake")
elseif("risv" STREQUAL ${IDF_TARGET_ARCH})
    include("${CMAKE_CURRENT_LIST_DIR}/arch/risv/.cmake")
else()
    message(FATAL_ERROR "unknown arch")
endif()

list(APPEND srcs
    "glist.c"
    "${IDF_TARGET}/startup.c"
    "${IDF_TARGET}/system_init.c"
    "esp_crosscore_int.c"
    "esp_dbg_ubsan.c"       # debug for undefined behavior, but not sure if its works
    "esp_freertos_hooks.c"
    "esp_platform.c"
    "esp_log.c"
    "${IDF_PATH}/components/esp_system/panic.c"
    "${IDF_PATH}/components/esp_system/port/panic_handler.c"
# compatiable
    "bootloader_support_shim.c"
)

list(APPEND includes
    "include"
    "include/bootloader_support"
    "include/dummy"
    #  NOTE fix: soc => ${IDF_TARGET} subdir using relative paths
    "${IDF_PATH}/components/soc/${IDF_TARGET}/include"
)
list(APPEND priv_includes
    "priv_include"
)

include("${CMAKE_CURRENT_LIST_DIR}/posix/.cmake")

# if(CONFIG_ESP_SYSTEM_USE_EH_FRAME)
#     list(APPEND srcs
#         "${IDF_PATH}/components/esp_system/eh_frame_parser.c"
#     )
# endif()

# if(CONFIG_SOC_SYSTIMER_SUPPORT_ETM)
#     list(APPEND srcs
#         "${IDF_PATH}/components/esp_system/systick_etm.c"
#     )
# endif()

idf_component_register(
    INCLUDE_DIRS
        ${includes}
    PRIV_INCLUDE_DIRS
        ${priv_includes}
    SRCS
        ${srcs}
    PRIV_REQUIRES
        "efuse"
        "hal"
        "esp_timer"
    LDFRAGMENTS
        "${IDF_PATH}/components/esp_common/common.lf"
        "${IDF_PATH}/components/esp_common/soc.lf"
        "memap.lf"
)
target_link_libraries(${COMPONENT_LIB} INTERFACE "-u __ubsan_include")

add_subdirectory("${IDF_TARGET}"
    "${build_dir}/${prefix}/${component_name}/soc")

# generate link scripts
idf_build_get_property(sdkconfig_header SDKCONFIG_HEADER)
idf_build_get_property(config_dir CONFIG_DIR)
set(ld_input "${CMAKE_CURRENT_LIST_DIR}/${IDF_TARGET}/memory.ld.in")
set(ld_output "${config_dir}/${IDF_TARGET}_memory.ld")

add_custom_command(
    OUTPUT ${ld_output}
    COMMAND "${CMAKE_C_COMPILER}" -C -P -x c -E -o ${ld_output} -I ${config_dir}
            -I "${CMAKE_CURRENT_LIST_DIR}" ${ld_input}
    MAIN_DEPENDENCY ${ld_input}
    DEPENDS ${sdkconfig_header}
    COMMENT "Generating memory.ld linker script..."
    VERBATIM
)

add_custom_target(memory_ld DEPENDS ${ld_output})
add_dependencies(${COMPONENT_LIB} memory_ld)

target_linker_script(${COMPONENT_LIB} INTERFACE "${ld_output}")
# Process the template file through the linker script generation mechanism, and use the output for linking the
# final binary
target_linker_script(${COMPONENT_LIB} INTERFACE "${IDF_TARGET}/sections.ld"
    PROCESS "${config_dir}/${IDF_TARGET}_sections.ld"
)

idf_component_optional_requires(PRIVATE "esp_gdbstub")
