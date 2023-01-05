list(APPEND includes
)

list(APPEND srcs
    "${IDF_TARGET}/startup.c"
    "${IDF_TARGET}/system_init.c"
    "${IDF_PATH}/components/esp_system/crosscore_int.c"
    "${IDF_PATH}/components/esp_system/esp_ipc.c"
    "${IDF_PATH}/components/esp_system/freertos_hooks.c"
    "${IDF_PATH}/components/esp_system/int_wdt.c"
    "${IDF_PATH}/components/esp_system/xt_wdt.c"
    "${IDF_PATH}/components/esp_system/debug_stubs.c"
    "${IDF_PATH}/components/esp_system/port/brownout.c"
    # "${IDF_PATH}/components/esp_system/port/esp_system_chip.c"
    "${IDF_PATH}/components/esp_system/panic.c"
    "${IDF_PATH}/components/esp_system/port/panic_handler.c"
)

if(CONFIG_ESP_SYSTEM_USE_EH_FRAME)
    list(APPEND srcs
        "${IDF_PATH}/components/esp_system/eh_frame_parser.c"
    )
endif()

if(CONFIG_SOC_SYSTIMER_SUPPORT_ETM)
    list(APPEND srcs
        "${IDF_PATH}/components/esp_system/systick_etm.c"
    )
endif()

idf_component_register(
    INCLUDE_DIRS
        "${IDF_PATH}/components/esp_system/include"
        "${IDF_PATH}/components/esp_system/port/include/private"
    PRIV_INCLUDE_DIRS
        "priv_include"
        "${IDF_PATH}/components/esp_system/port/include"
    SRCS
        ${srcs}
    PRIV_REQUIRES
        "efuse"
        "hal"
        "esp_common"
        "esp_timer"
    LDFRAGMENTS
        "${IDF_PATH}/components/esp_system/linker.lf"
        "${IDF_PATH}/components/esp_system/app.lf"
)
set_source_files_properties(heap.c PROPERTIES COMPILE_FLAGS -fno-builtin)

add_subdirectory("${IDF_PATH}/components/esp_system/port/soc/${IDF_TARGET}"
    "${build_dir}/${prefix}/${component_name}/soc")

# Force linking UBSAN hooks. If UBSAN is not enabled, the hooks will ultimately be removed
# due to -ffunction-sections -Wl,--gc-sections options.
target_link_libraries(${COMPONENT_LIB} INTERFACE "-u __ubsan_include")

if(NOT CONFIG_ESP_SYSTEM_SINGLE_CORE_MODE)
    target_link_libraries(${COMPONENT_LIB} INTERFACE "-u start_app_other_cores")
endif()

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
