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
)

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
)
add_subdirectory(${component_path}/port "${build_dir}/${prefix}/${component_name}/port")

list(APPEND scripts
    "ld/${IDF_TARGET}.ld"
    "${IDF_PATH}/components/bootloader/subproject/main/ld/${IDF_TARGET}/bootloader.rom.ld"
)
target_linker_script(${COMPONENT_LIB} INTERFACE "${scripts}")


# if(CONFIG_IDF_ENV_FPGA)
#     # Forces the linker to include fpga stubs from this component
#     target_link_libraries(${COMPONENT_LIB} INTERFACE "-u esp_common_include_fpga_overrides")
# endif()

# # Force linking UBSAN hooks. If UBSAN is not enabled, the hooks will ultimately be removed
# # due to -ffunction-sections -Wl,--gc-sections options.
# target_link_libraries(${COMPONENT_LIB} INTERFACE "-u __ubsan_include")

#     list(APPEND srcs
#             "esp_system.c"
#             "startup.c"
#             "system_time.c"
#             "stack_check.c"
#             "ubsan.c"
#             "xt_wdt.c"
#             "debug_stubs.c")

#     if(CONFIG_ESP_TASK_WDT_EN)
#         list(APPEND srcs "task_wdt/task_wdt.c")

#         if(CONFIG_ESP_TASK_WDT_USE_ESP_TIMER)
#             list(APPEND srcs "task_wdt/task_wdt_impl_esp_timer.c")
#         else()
#             list(APPEND srcs "task_wdt/task_wdt_impl_timergroup.c")
#         endif()
#     endif()

#     if(CONFIG_ESP_SYSTEM_USE_EH_FRAME)
#         list(APPEND srcs "eh_frame_parser.c")
#     endif()

#     idf_component_register(SRCS "${srcs}"
#                         INCLUDE_DIRS include
#                         PRIV_REQUIRES spi_flash esp_timer
#                                     # [refactor-todo] requirements due to init code,
#                                     # should be removable once using component init functions
#                                     # link-time registration is used.
#                                     # [refactor-todo] requires "driver" for headers:
#                                     # - spi_common_internal.h
#                                     # [refactor-todo] esp_partition required for virtual efuse
#                                     # init code. Move to esp_efuse component.
#                                     pthread bootloader_support efuse driver esp_partition
#                         LDFRAGMENTS "linker.lf" "app.lf")
#     add_subdirectory(port)

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

idf_component_optional_requires(PRIVATE esp_app_format)

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
