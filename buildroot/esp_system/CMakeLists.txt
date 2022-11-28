set(component_path "${IDF_PATH}/components/esp_system")

if(CONFIG_IDF_ENV_FPGA)
    list(APPEND srcs "fpga_overrides.c")
endif()

list(APPEND srcs
    "startup.c"
    "log.c"
    "libc_retarget.c"
    "${component_path}/esp_err.c"
    "${component_path}/crosscore_int.c"
    "${component_path}/int_wdt.c"
    "${component_path}/esp_ipc.c"
    "${component_path}/freertos_hooks.c"
    "${component_path}/panic.c"
    "${component_path}/esp_system.c"
    "${component_path}/system_time.c"
    "${component_path}/stack_check.c"
    "${component_path}/ubsan.c"
    "${component_path}/xt_wdt.c"
    "${component_path}/debug_stubs.c"
)

if(CONFIG_ESP_SYSTEM_USE_EH_FRAME)
    list(APPEND srcs
        "${component_path}/eh_frame_parser.c"
    )
endif()

if(CONFIG_ESP_TASK_WDT_EN)
    list(APPEND srcs
        "${component_path}/task_wdt/task_wdt.c"
    )

    if(CONFIG_ESP_TASK_WDT_USE_ESP_TIMER)
        list(APPEND srcs
            "${component_path}/task_wdt/task_wdt_impl_esp_timer.c"
        )
    else()
        list(APPEND srcs
            "${component_path}/task_wdt/task_wdt_impl_timergroup.c"
        )
    endif()
endif()

idf_component_register(
    INCLUDE_DIRS
        "include"
        "posix"
        "${IDF_PATH}/components/esp_system/include"
    PRIV_INCLUDE_DIRS
        "${component_path}/port/include"
    SRCS
        "${srcs}"
    REQUIRES
        "freertos"
        "bootloader_support"
        "soc" "hal" "efuse"
        "spi_flash"
        "esp_hw_support"
    # LDFRAGMENTS
    #     "${component_path}/linker.lf"
    #     "${component_path}/app.lf"
)

add_subdirectory(${component_path}/port "${build_dir}/${prefix}/${component_name}/port")

idf_build_get_property(config_dir CONFIG_DIR)
idf_build_get_property(sdkconfig_header SDKCONFIG_HEADER)
set(memap_ld_input "${CMAKE_CURRENT_LIST_DIR}/ld/${IDF_TARGET}.memap.ld.in")
set(memap_ld_output "${config_dir}/${IDF_TARGET}.memap.ld")

# Preprocess target.memap.ld.in linker script to include configuration, becomes target.memap.ld
add_custom_command(
    OUTPUT ${memap_ld_output}
    COMMAND "${CMAKE_C_COMPILER}" -C -P -x c -E -o ${memap_ld_output} -I ${config_dir}
        -I "${CMAKE_CURRENT_LIST_DIR}" ${memap_ld_input}
    MAIN_DEPENDENCY ${memap_ld_input}
    DEPENDS ${sdkconfig_header}
    COMMENT "Generating memory.ld linker script..."
    VERBATIM
)
add_custom_target(generate_mem_ld DEPENDS ${memap_ld_output})
add_dependencies(${COMPONENT_LIB} generate_mem_ld)

list(APPEND scripts
    "${memap_ld_output}"
    "ld/${IDF_TARGET}.sections.ld"
    # "${IDF_PATH}/components/bootloader/subproject/main/ld/${IDF_TARGET}/bootloader.rom.ld"
)
target_linker_script(${COMPONENT_LIB} INTERFACE "${scripts}")

# list(APPEND scripts
#     "ld/${IDF_TARGET}.ld"
#     "${IDF_PATH}/components/bootloader/subproject/main/ld/${IDF_TARGET}/bootloader.rom.ld"
# )
# target_linker_script(${COMPONENT_LIB} INTERFACE "${scripts}")

# if(CONFIG_IDF_ENV_FPGA)
#     # Forces the linker to include fpga stubs from this component
#     target_link_libraries(${COMPONENT_LIB} INTERFACE "-u esp_common_include_fpga_overrides")
# endif()

#     # After system initialization, `start_app` (and its other cores variant) is called.
#     # This is provided by the user or from another component. Since we can't establish
#     # dependency on what we don't know, force linker to not drop the symbol regardless
#     # of link line order.
#     target_link_libraries(${COMPONENT_LIB} INTERFACE "-u start_app")

#     if(NOT CONFIG_ESP_SYSTEM_SINGLE_CORE_MODE)
#         target_link_libraries(${COMPONENT_LIB} INTERFACE "-u start_app_other_cores")
#     endif()

#     # Disable stack protection in files which are involved in initialization of that feature
#     set_source_files_properties(
#         "startup.c" "stack_check.c"
#         PROPERTIES COMPILE_FLAGS
#         -fno-stack-protector)

#     include(${CMAKE_CURRENT_LIST_DIR}/ld/ld.cmake)
# endif()

# [refactor-todo] requirements due to init code, should be removable
# once link-time registration of component init functions is used.
if(CONFIG_APPTRACE_ENABLE)
    idf_component_optional_requires(PRIVATE app_trace)
endif()

if(CONFIG_ESP_COREDUMP_ENABLE)
    idf_component_optional_requires(PRIVATE espcoredump)
endif()

# [refactor-todo] requirement from the panic handler,
# need to introduce panic "event" concept to remove this dependency (IDF-2194)
idf_component_optional_requires(PRIVATE esp_gdbstub)

if(CONFIG_PM_ENABLE)
    idf_component_optional_requires(PRIVATE pm)
endif()

if(CONFIG_VFS_SUPPORT_IO)
    idf_component_optional_requires(PRIVATE vfs)
endif()

if(CONFIG_SW_COEXIST_ENABLE OR CONFIG_EXTERNAL_COEX_ENABLE)
    idf_component_optional_requires(PRIVATE esp_wifi)
endif()

if(CONFIG_SPIRAM)
    idf_component_optional_requires(PRIVATE esp_psram)
endif()
