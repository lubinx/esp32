/*
 * SPDX-FileCopyrightText: 2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stddef.h>
#include <assert.h>

#include "esp_task.h"
#include "esp_log.h"
#include "esp_freertos_hooks.h"
#include "esp_private/crosscore_int.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/portmacro.h"

#include "hal/systimer_hal.h"
#include "hal/systimer_ll.h"

#include "sdkconfig.h"

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

/****************************************************************************
 *  @implements: esp_system.h
*****************************************************************************/
void esp_system_abort(const char *details)
{
    abort();
}

esp_err_t esp_register_shutdown_handler(void (*function)(void))
{
    return atexit(function);
}

uint32_t esp_get_free_heap_size(void)
{
    return heap_caps_get_free_size(MALLOC_CAP_DEFAULT);
}

/****************************************************************************
 *  @implements: esp_cpu.h
*****************************************************************************/
esp_err_t esp_cpu_set_watchpoint(int wp_nb, const void *wp_addr, size_t size, esp_cpu_watchpoint_trigger_t trigger)
{
    assert(SOC_CPU_WATCHPOINTS_NUM > 0);

    /*
    Todo:
    - Check that wp_nb is in range
    - Check if the wp_nb is already in use
    */
    // Check if size is 2^n, where n is in [0...6]
    if (size < 1 || size > 64 || (size & (size - 1)) != 0) {
        return ESP_ERR_INVALID_ARG;
    }
    int on_read = (trigger == ESP_CPU_WATCHPOINT_LOAD || trigger == ESP_CPU_WATCHPOINT_ACCESS);
    int on_write = (trigger == ESP_CPU_WATCHPOINT_STORE || trigger == ESP_CPU_WATCHPOINT_ACCESS);
#if __XTENSA__
    xt_utils_set_watchpoint(wp_nb, (uint32_t)wp_addr, size, on_read, on_write);
#else
    if (__dbgr_is_attached()) {
        // See description in esp_cpu_set_breakpoint()
        long args[] = {true, wp_nb, (long)wp_addr, (long)size,
            (long)((on_read ? ESP_SEMIHOSTING_WP_FLG_RD : 0) | (on_write ? ESP_SEMIHOSTING_WP_FLG_WR : 0))
        };

        if (0 == semihosting_call_noerrno(ESP_SEMIHOSTING_SYS_WATCHPOINT_SET, args))
            return ESP_ERR_INVALID_RESPONSE;
    }
    rv_utils_set_watchpoint(wp_nb, (uint32_t)wp_addr, size, on_read, on_write);
#endif
    return ESP_OK;
}

esp_err_t esp_cpu_clear_watchpoint(int wp_nb)
{
    /*
    Todo:
    - Check if the wp_nb is valid
    */
#if __XTENSA__
    xt_utils_clear_watchpoint(wp_nb);
#else
    if (__dbgr_is_attached())
    {
        // See description in __dbgr_is_attached()
        long args[] = {false, wp_nb};

        if (0 == semihosting_call_noerrno(ESP_SEMIHOSTING_SYS_WATCHPOINT_SET, args))
            return ESP_ERR_INVALID_RESPONSE;
    }
    rv_utils_clear_watchpoint(wp_nb);
#endif // __XTENSA__
    return ESP_OK;
}

/****************************************************************************
 *  @implements: hal/systimer_hal.h
*****************************************************************************/
void systimer_hal_init(systimer_hal_context_t *hal)
{
    hal->dev = &SYSTIMER;
    systimer_ll_enable_clock(hal->dev, true);
}

void systimer_hal_counter_can_stall_by_cpu(systimer_hal_context_t *hal, uint32_t counter_id, uint32_t cpu_id, bool can)
{
    systimer_ll_counter_can_stall_by_cpu(hal->dev, counter_id, cpu_id, can);
}

void systimer_hal_enable_counter(systimer_hal_context_t *hal, uint32_t counter_id)
{
    systimer_ll_enable_counter(hal->dev, counter_id, true);
}

void systimer_hal_connect_alarm_counter(systimer_hal_context_t *hal, uint32_t alarm_id, uint32_t counter_id)
{
    systimer_ll_connect_alarm_counter(hal->dev, alarm_id, counter_id);
}

void systimer_hal_set_alarm_period(systimer_hal_context_t *hal, uint32_t alarm_id, uint32_t period)
{
    systimer_ll_enable_alarm(hal->dev, alarm_id, false);
    systimer_ll_set_alarm_period(hal->dev, alarm_id, hal->us_to_ticks(period));
    systimer_ll_apply_alarm_value(hal->dev, alarm_id);
    systimer_ll_enable_alarm(hal->dev, alarm_id, true);
}

void systimer_hal_select_alarm_mode(systimer_hal_context_t *hal, uint32_t alarm_id, systimer_alarm_mode_t mode)
{
    switch (mode) {
    case SYSTIMER_ALARM_MODE_ONESHOT:
        systimer_ll_enable_alarm_oneshot(hal->dev, alarm_id);
        break;
    case SYSTIMER_ALARM_MODE_PERIOD:
        systimer_ll_enable_alarm_period(hal->dev, alarm_id);
        break;
    default:
        break;
    }
}

void systimer_hal_enable_alarm_int(systimer_hal_context_t *hal, uint32_t alarm_id)
{
    systimer_ll_enable_alarm_int(hal->dev, alarm_id, true);
}

uint64_t systimer_hal_get_counter_value(systimer_hal_context_t *hal, uint32_t counter_id)
{
    uint32_t lo, lo_start, hi;
    /* Set the "update" bit and wait for acknowledgment */
    systimer_ll_counter_snapshot(hal->dev, counter_id);
    while (!systimer_ll_is_counter_value_valid(hal->dev, counter_id));
    /* Read LO, HI, then LO again, check that LO returns the same value.
     * This accounts for the case when an interrupt may happen between reading
     * HI and LO values, and this function may get called from the ISR.
     * In this case, the repeated read will return consistent values.
     */
    lo_start = systimer_ll_get_counter_value_low(hal->dev, counter_id);
    do {
        lo = lo_start;
        hi = systimer_ll_get_counter_value_high(hal->dev, counter_id);
        lo_start = systimer_ll_get_counter_value_low(hal->dev, counter_id);
    } while (lo_start != lo);

    return (uint64_t)hi << 32 | lo;
}

void systimer_hal_counter_value_advance(systimer_hal_context_t *hal, uint32_t counter_id, int64_t time_us)
{
    systimer_ll_set_counter_value(hal->dev, counter_id, systimer_hal_get_counter_value(hal, counter_id) + hal->us_to_ticks(time_us));
    systimer_ll_apply_counter_value(hal->dev, counter_id);
}

