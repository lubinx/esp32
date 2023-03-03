set(srcs
    "kernel-smp/list.c"
    "kernel-smp/queue.c"
    "kernel-smp/tasks.c"
    "kernel-smp/timers.c"
    "kernel-smp/croutine.c"
    "kernel-smp/event_groups.c"
    "kernel-smp/stream_buffer.c"
    "kernel-smp/portable/${IDF_TARGET_ARCH}/port.c"
    "kernel-smp/portable/${IDF_TARGET_ARCH}/portasm.S"
    "port_common.c"
)

set(include_dirs
    "include"                           # For files with #include "freertos/FreeRTOSConfig.h"
    "include/freertos"                  # For files with #include "FreeRTOSConfig.h"
    "kernel-smp/include"
    "kernel-smp/portable/${IDF_TARGET_ARCH}/include"
)

set(private_include_dirs
    "include"
    "private_include"                   # For include "freertos_tasks_c_additions.h"
    "kernel-smp/portable/${IDF_TARGET_ARCH}/include/freertos"
    "kernel-smp/portable/${IDF_TARGET_ARCH}"
    "kernel-smp/include/freertos"    # FreeRTOS headers via #include "xxx.h"
)

if(CONFIG_FREERTOS_SMP)
    list(APPEND include_dirs "kernel-smp/portable/${IDF_TARGET_ARCH}/include/freertos")  # Xtensa headers via #include "xx.h"
else()
    list(APPEND srcs "kernel-smp/portable/port_systick.c")
    list(APPEND private_include_dirs "kernel-smp/portable/priv_include")                 # For port_systick.h on normal FreeRTOS
endif()

if(CONFIG_IDF_TARGET_ARCH_XTENSA)
    list(APPEND srcs
        "kernel-smp/portable/xtensa/xtensa_context.S"
        "kernel-smp/portable/xtensa/xtensa_init.c"
        "kernel-smp/portable/xtensa/xtensa_overlay_os_hook.c"
        "kernel-smp/portable/xtensa/xtensa_vector_defaults.S"
        "kernel-smp/portable/xtensa/xtensa_vectors.S"
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
