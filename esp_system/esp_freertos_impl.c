/*
 * SPDX-FileCopyrightText: 2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stddef.h>
#include <assert.h>

#include "esp_task.h"
#include "esp_attr.h"
#include "esp_log.h"
#include "esp_freertos_hooks.h"
#include "esp_private/crosscore_int.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/portmacro.h"

/****************************************************************************
 *  @implements: freertos tick & idle
*****************************************************************************/
void IRAM_ATTR vApplicationTickHook(void)
{
    // nothing to do
}

void IRAM_ATTR vApplicationIdleHook(void)
{
    __WFI();
}

void IRAM_ATTR esp_vApplicationTickHook(void)
{
    // override to remove
}

void IRAM_ATTR esp_vApplicationIdleHook(void)
{
    // override to remove
}

/****************************************************************************
 *  @implements: freertos main task
*****************************************************************************/
static void freertos_main_task(void *args)
{
    // TODO: process main() exit code
    extern void main(void);
    main();

    vTaskDelete(NULL);
}

void esp_startup_start_app(void)
{
    // Initialize the cross-core interrupt on CPU0
    esp_crosscore_int_init();

    /*
    BaseType_t res = xTaskCreatePinnedToCore(freertos_main_task, "main",
        ESP_TASK_MAIN_STACK, NULL,
        ESP_TASK_MAIN_PRIO, NULL, ESP_TASK_MAIN_CORE
    );
    assert(res == pdTRUE);
    (void)res;
    */
    BaseType_t res = xTaskCreate(freertos_main_task, "app_main", ESP_TASK_MAIN_STACK, NULL, ESP_TASK_MAIN_PRIO, NULL);
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

    xPortStartScheduler();
    abort(); // Only get to here if FreeRTOS somehow very broken
}
#endif // !CONFIG_FREERTOS_UNICORE

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
#include "hal/systimer_hal.h"
#include "hal/systimer_ll.h"

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
