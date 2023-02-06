/*
 * SPDX-FileCopyrightText: 2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "sdkconfig.h"
#include <stddef.h>
#include <assert.h>
#include "esp_task.h"
#include "esp_log.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/portmacro.h"

#include "esp_private/crosscore_int.h"
#include "esp_freertos_hooks.h"
#include "esp_heap_caps_init.h"

#ifdef CONFIG_APPTRACE_ENABLE
    #include "esp_app_trace.h"                  /* Required for esp_apptrace_init. [refactor-todo] */
#endif
#ifdef CONFIG_ESP_SYSTEM_GDBSTUB_RUNTIME
    #include "esp_gdbstub.h"                    /* Required by esp_gdbstub_init() */
#endif

/* ------------------------------------------------- App/OS Startup ----------------------------------------------------
 * - Functions related to application and FreeRTOS startup
 * - This startup is common to all architectures (e.g. RISC-V and Xtensa) and all FreeRTOS implementations (i.e., IDF
 *   FreeRTOS and Amazon SMP FreeRTOS).
 * - Application startup flow as follows:
 *      - For CPU 0
 *          - CPU0 completes CPU startup (in startup.c), then calls esp_startup_start_app()
 *          - esp_startup_start_app() registers some daemon services for CPU0 then starts FreeRTOS
 *      - For CPUx (1 to N-1)
 *          - CPUx completes CPU startup in startup.c, then calls esp_startup_start_app_other_cores()
 *          - esp_startup_start_app_other_cores(), registers some daemon services for CPUx, waits for CPU0 to start
 *            FreeRTOS, then yields (via xPortStartScheduler()) to schedule a task.
 * ------------------------------------------------------------------------------------------------------------------ */

// ----------------------- Checks --------------------------

/*
For now, AMP is not supported (i.e., running FreeRTOS on one core and a bare metal/other OS on the other). Therefore,
CONFIG_FREERTOS_UNICORE and CONFIG_ESP_SYSTEM_SINGLE_CORE_MODE should be identical. We add a check for this here.
*/
#if CONFIG_FREERTOS_UNICORE != CONFIG_ESP_SYSTEM_SINGLE_CORE_MODE
    #error "AMP not supported. FreeRTOS number of cores and system number of cores must be identical"
#endif

// -------------------- Declarations -----------------------

static void main_task(void *args);
static const char *APP_START_TAG = "app_start";

// ------------------ CPU0 App Startup ---------------------

void esp_startup_start_app(void)
{
    // Initialize the cross-core interrupt on CPU0
    esp_crosscore_int_init();

    #if CONFIG_ESP_SYSTEM_GDBSTUB_RUNTIME && !CONFIG_IDF_TARGET_ESP32C2
        void esp_gdbstub_init(void);
        esp_gdbstub_init();
    #endif // CONFIG_ESP_SYSTEM_GDBSTUB_RUNTIME

    /*
    BaseType_t res = xTaskCreatePinnedToCore(main_task, "main",
        ESP_TASK_MAIN_STACK, NULL,
        ESP_TASK_MAIN_PRIO, NULL, ESP_TASK_MAIN_CORE
    );
    assert(res == pdTRUE);
    (void)res;
    */
    BaseType_t res = xTaskCreate(main_task, "main", ESP_TASK_MAIN_STACK, NULL, ESP_TASK_MAIN_PRIO, NULL);

    ESP_EARLY_LOGI(APP_START_TAG, "Starting scheduler on CPU0");
    vTaskStartScheduler();
}

// --------------- CPU[1:N-1] App Startup ------------------

#if !CONFIG_FREERTOS_UNICORE
void esp_startup_start_app_other_cores(void)
{
    // For now, we only support up to two core: 0 and 1.
    if (xPortGetCoreID() >= 2) {
        abort();
    }

    // Wait for CPU0 to start FreeRTOS before progressing
    extern volatile unsigned port_xSchedulerRunning[portNUM_PROCESSORS];
    while (port_xSchedulerRunning[0] == 0) {
        ;
    }

#if CONFIG_APPTRACE_ENABLE
    // [refactor-todo] move to esp_system initialization
    esp_err_t err = esp_apptrace_init();
    assert(err == ESP_OK && "Failed to init apptrace module on APP CPU!");
#endif

    // Initialize the cross-core interrupt on CPU1
    esp_crosscore_int_init();

    ESP_EARLY_LOGI(APP_START_TAG, "Starting scheduler on CPU%d", xPortGetCoreID());
    xPortStartScheduler();
    abort(); // Only get to here if FreeRTOS somehow very broken
}
#endif // !CONFIG_FREERTOS_UNICORE

/* ---------------------------------------------------- Main Task ------------------------------------------------------
 * - main_task is a daemon task created by CPU0 before it starts FreeRTOS
 *      - Pinned to CPU(ESP_TASK_MAIN_CORE)
 *      - Priority of ESP_TASK_MAIN_PRIO
 * - Used to dispatch "void app_main(void)" provided by the application
 * - main_task will self delete if app_main returns
 * ------------------------------------------------------------------------------------------------------------------ */

static const char *MAIN_TAG = "main_task";

#if !CONFIG_FREERTOS_UNICORE
static volatile bool s_other_cpu_startup_done = false;
static bool other_cpu_startup_idle_hook_cb(void)
{
    s_other_cpu_startup_done = true;
    return true;
}
#endif

static void main_task(void *args)
{
    ESP_LOGI(MAIN_TAG, "Started on CPU%d", xPortGetCoreID());

#if !CONFIG_FREERTOS_UNICORE
    // Wait for FreeRTOS initialization to finish on other core, before replacing its startup stack
    esp_register_freertos_idle_hook_for_cpu(other_cpu_startup_idle_hook_cb, !xPortGetCoreID());
    while (!s_other_cpu_startup_done) {
        ;
    }
    esp_deregister_freertos_idle_hook_for_cpu(other_cpu_startup_idle_hook_cb, !xPortGetCoreID());
#endif

    // [refactor-todo] check if there is a way to move the following block to esp_system startup
    // heap_caps_enable_nonos_stack_heaps();

    /*
    Note: Be careful when changing the "Calling app_main()" log below as multiple pytest scripts expect this log as a
    start-of-application marker.
    */
    ESP_LOGI(MAIN_TAG, "Calling app_main()");
    extern void app_main(void);
    app_main();
    ESP_LOGI(MAIN_TAG, "Returned from app_main()");
    vTaskDelete(NULL);
}
