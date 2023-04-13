set(srcs
    "freertos-smp/list.c"
    "freertos-smp/queue.c"
    "freertos-smp/tasks.c"
    "freertos-smp/timers.c"
    "freertos-smp/croutine.c"
    "freertos-smp/event_groups.c"
    "freertos-smp/stream_buffer.c"
    # "port_common.c"
)

set(include_dirs
    "include"                           # For files with #include "freertos/FreeRTOSConfig.h"
    "xtensa/include"
    "freertos-smp/include"
)

set(private_include_dirs
    "private_include"
    # "kernel-smp/include/freertos"       # FreeRTOS headers via #include "xxx.h"
)

if(CONFIG_IDF_TARGET_ARCH_XTENSA)
    list(APPEND srcs
        "xtensa/port.c"
        "xtensa/portasm.S"
        "xtensa/xtensa_context.S"
        "xtensa/xtensa_vector_defaults.S"
        "xtensa/xtensa_vectors.S"
        # CONFIG_ESP32_IRAM_AS_8BIT_ACCESSIBLE_MEMORY
        "xtensa/xtensa_loadstore_handler.S"
        # esp-idf
        # "xtensa/xtensa_overlay_os_hook.c"
        # "esp_ringbuf.c"     # TODO: CMAKE: esp_ringbuf remove later
    )
endif()

idf_component_register(
    INCLUDE_DIRS
        ${include_dirs}
    PRIV_INCLUDE_DIRS
        ${private_include_dirs}
    SRCS
        ${srcs}
    REQUIRES
        "esp_common"
        "soc"
        "heap"
)
target_compile_options(${COMPONENT_LIB} PRIVATE
    "-Wno-conversion"
    "-Wno-unused-parameter"
    "-Wno-sign-compare"
    "-Wno-type-limits"
)

if(CONFIG_FREERTOS_ENABLE_STATIC_TASK_CLEAN_UP AND CONFIG_FREERTOS_SMP)
    target_link_libraries(${COMPONENT_LIB} INTERFACE "-Wl,--wrap=vPortCleanUpTCB")
endif()
