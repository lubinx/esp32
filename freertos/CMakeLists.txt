set(srcs
    "kernel-smp/list.c"
    "kernel-smp/queue.c"
    "kernel-smp/tasks.c"
    "kernel-smp/timers.c"
    "kernel-smp/croutine.c"
    "kernel-smp/event_groups.c"
    "kernel-smp/stream_buffer.c"
    "port_common.c"
)

set(include_dirs
    "include"                           # For files with #include "freertos/FreeRTOSConfig.h"
    "port_xtensa/include"
    "kernel-smp/include"
)

set(private_include_dirs
    "private_include"
    "kernel-smp/include/freertos"       # FreeRTOS headers via #include "xxx.h"
)

if(CONFIG_IDF_TARGET_ARCH_XTENSA)
    list(APPEND srcs
        "port_xtensa/port.c"
        "port_xtensa/portasm.S"
        "port_xtensa/xtensa_context.S"
        "port_xtensa/xtensa_overlay_os_hook.c"
        "port_xtensa/xtensa_vector_defaults.S"
        "port_xtensa/xtensa_vectors.S"
        # "kernel-smp/portable/xtensa/xtensa_loadstore_handler.S"                          # CONFIG_ESP32_IRAM_AS_8BIT_ACCESSIBLE_MEMORY
    )
endif()

idf_component_register(
    INCLUDE_DIRS
        ${include_dirs}
    PRIV_INCLUDE_DIRS
        ${private_include_dirs}
    SRCS
        ${srcs}
    PRIV_REQUIRES
        "esp_common"
        "soc"
)

if(CONFIG_FREERTOS_ENABLE_STATIC_TASK_CLEAN_UP AND CONFIG_FREERTOS_SMP)
    target_link_libraries(${COMPONENT_LIB} INTERFACE "-Wl,--wrap=vPortCleanUpTCB")
endif()
