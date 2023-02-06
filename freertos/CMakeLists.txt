if(CONFIG_FREERTOS_SMP)
    set(kernel_dir "${IDF_PATH}/components/freertos/FreeRTOS-Kernel-SMP")
else()
    set(kernel_dir "${IDF_PATH}/components/freertos/FreeRTOS-Kernel")
endif()

set(srcs
    "${kernel_dir}/list.c"
    "${kernel_dir}/queue.c"
    "${kernel_dir}/tasks.c"
    "${kernel_dir}/timers.c"
    "${kernel_dir}/croutine.c"
    "${kernel_dir}/event_groups.c"
    "${kernel_dir}/stream_buffer.c"
    "${kernel_dir}/portable/${IDF_TARGET_ARCH}/port.c"
    "${kernel_dir}/portable/${IDF_TARGET_ARCH}/portasm.S"
    "port_common.c"
    # esp
    "esp_freertos_hooks.c"
)
set_source_files_properties(
    tasks.c
    event_groups.c
    timers.c
    queue.c
    stream_buffer.c
    PROPERTIES COMPILE_DEFINITIONS
    _ESP_FREERTOS_INTERNAL
)

set(include_dirs
    "include"             # For files with #include "freertos/FreeRTOSConfig.h"
    "include/freertos"    # For files with #include "FreeRTOSConfig.h"
    "${kernel_dir}/include"
    "${kernel_dir}/portable/${IDF_TARGET_ARCH}/include"
)

set(private_include_dirs
    "include"
    "private_include"    # For include "freertos_tasks_c_additions.h"
    "${kernel_dir}/portable/${IDF_TARGET_ARCH}/include/freertos"
    "${kernel_dir}/portable/${IDF_TARGET_ARCH}"
    "${kernel_dir}/include/freertos"   # FreeRTOS headers via #include "xxx.h"
)

if(CONFIG_FREERTOS_SMP)
    set(ldfragments linker_smp.lf)
    list(APPEND include_dirs "${kernel_dir}/portable/${IDF_TARGET_ARCH}/include/freertos")  # Xtensa headers via #include "xx.h"
else()
    set(ldfragments linker.lf)
    list(APPEND srcs "${kernel_dir}/portable/port_systick.c")
    list(APPEND private_include_dirs "${kernel_dir}/portable/priv_include")                 # For port_systick.h on normal FreeRTOS
endif()

if(CONFIG_IDF_TARGET_ARCH_XTENSA)
    list(APPEND srcs
        "${kernel_dir}/portable/xtensa/xtensa_context.S"
        "${kernel_dir}/portable/xtensa/xtensa_init.c"
        "${kernel_dir}/portable/xtensa/xtensa_overlay_os_hook.c"
        "${kernel_dir}/portable/xtensa/xtensa_vector_defaults.S"
        "${kernel_dir}/portable/xtensa/xtensa_vectors.S"
        "${kernel_dir}/portable/xtensa/xtensa_loadstore_handler.S"  # CONFIG_ESP32_IRAM_AS_8BIT_ACCESSIBLE_MEMORY
    )
endif()

idf_component_register(
    SRCS
        ${srcs}
    INCLUDE_DIRS
        ${include_dirs}
    PRIV_INCLUDE_DIRS
        ${private_include_dirs}
    # LDFRAGMENTS               # nothing is required into RAM
    #     ${ldfragments}
    PRIV_REQUIRES
        "soc"
)

if(CONFIG_FREERTOS_DEBUG_OCDAWARE)
    target_link_libraries(${COMPONENT_LIB} INTERFACE "-Wl,--undefined=FreeRTOS_openocd_params")
    idf_build_set_property(COMPILE_OPTIONS "-DconfigENABLE_FREERTOS_DEBUG_OCDAWARE=1" APPEND)
endif()

if(CONFIG_FREERTOS_ENABLE_STATIC_TASK_CLEAN_UP AND CONFIG_FREERTOS_SMP)
    target_link_libraries(${COMPONENT_LIB} INTERFACE "-Wl,--wrap=vPortCleanUpTCB")
endif()
