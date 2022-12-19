list(APPEND includes
)

list(APPEND srcs
    "${IDF_TARGET}/init.c"
    "startup.c"
    "${IDF_PATH}/components/esp_system/crosscore_int.c"
    "${IDF_PATH}/components/esp_system/esp_ipc.c"
    "${IDF_PATH}/components/esp_system/esp_err.c"
    "${IDF_PATH}/components/esp_system/freertos_hooks.c"
    "${IDF_PATH}/components/esp_system/int_wdt.c"
    "${IDF_PATH}/components/esp_system/esp_system.c"
    "${IDF_PATH}/components/esp_system/panic.c"
    # "${IDF_PATH}/components/esp_system/system_time.c"
    # "${IDF_PATH}/components/esp_system/stack_check.c"
    "${IDF_PATH}/components/esp_system/ubsan.c"
    "${IDF_PATH}/components/esp_system/xt_wdt.c"
    "${IDF_PATH}/components/esp_system/debug_stubs.c"
    "${IDF_PATH}/components/esp_system/port/brownout.c"
    "${IDF_PATH}/components/esp_system/port/esp_system_chip.c"
    "${IDF_PATH}/components/esp_system/port/panic_handler.c"
    # "${IDF_PATH}/components/esp_system/startup.c"
    # "${IDF_PATH}/components/esp_system/port/cpu_start.c"
    # "${IDF_PATH}/components/bootloader_support/bootloader_flash/src/bootloader_flash.c"
)

if(CONFIG_ESP_TASK_WDT_EN)
    list(APPEND srcs
        "${IDF_PATH}/components/esp_system/task_wdt/task_wdt.c"
    )

    if(CONFIG_ESP_TASK_WDT_USE_ESP_TIMER)
        list(APPEND srcs
            "${IDF_PATH}/components/esp_system/task_wdt/task_wdt_impl_esp_timer.c"
        )
    else()
        list(APPEND srcs
            "${IDF_PATH}/components/esp_system/task_wdt/task_wdt_impl_timergroup.c"
        )
    endif()
endif()

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
        "${IDF_PATH}/components/esp_system/port/include"
        "${IDF_PATH}/components/bootloader_support/include"
        "${IDF_PATH}/components/spi_flash/include"
    SRCS
        ${srcs}
    PRIV_REQUIRES
        "efuse"
        "hal"
        "esp_common"
        "esp_timer"
        # "pthread"
        # "driver"
        # "esp_partition"
        # "spi_flash"
    LDFRAGMENTS
        "${IDF_PATH}/components/esp_system/linker.lf"
        "${IDF_PATH}/components/esp_system/app.lf"
)
add_subdirectory("${IDF_PATH}/components/esp_system/port/soc/${IDF_TARGET}"
    "${build_dir}/${prefix}/${component_name}/soc")

if(CONFIG_IDF_ENV_FPGA)
    # Forces the linker to include fpga stubs from this component
    target_link_libraries(${COMPONENT_LIB} INTERFACE "-u esp_common_include_fpga_overrides")
endif()

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

# Disable stack protection in files which are involved in initialization of that feature
# set_source_files_properties(
#     "${IDF_PATH}/components/esp_system/stack_check.c"
#     PROPERTIES COMPILE_FLAGS
#     -fno-stack-protector
# )

# [refactor-todo] requirement from the panic handler,
# need to introduce panic "event" concept to remove this dependency (IDF-2194)
idf_component_optional_requires(PRIVATE "esp_gdbstub")

# if(CONFIG_APPTRACE_ENABLE)
#     idf_component_optional_requires(PRIVATE "app_trace")
# endif()

# if(CONFIG_ESP_COREDUMP_ENABLE)
#     idf_component_optional_requires(PRIVATE "espcoredump")
# endif()

# if(CONFIG_PM_ENABLE)
#     idf_component_optional_requires(PRIVATE "pm")
# endif()

# if(CONFIG_VFS_SUPPORT_IO)
#     idf_component_optional_requires(PRIVATE "vfs")
# endif()

# if(CONFIG_SW_COEXIST_ENABLE OR CONFIG_EXTERNAL_COEX_ENABLE)
#     idf_component_optional_requires(PRIVATE "esp_wifi")
# endif()

# if(NOT BOOTLOADER_BUILD)
#     if(CONFIG_SPIRAM)
#         idf_component_optional_requires(PRIVATE "esp_psram")
#     endif()
# endif()