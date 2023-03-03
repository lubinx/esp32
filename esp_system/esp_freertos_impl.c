/*
 * SPDX-FileCopyrightText: 2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stddef.h>
#include <assert.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "clk_tree.h"
#include "esp_attr.h"
#include "esp_log.h"
#include "esp_freertos_hooks.h"
#include "esp_private/crosscore_int.h"

#include "hal/systimer_ll.h"
#include "hal/systimer_hal.h"

#include "sdkconfig.h"

/****************************************************************************
 *  @internal
*****************************************************************************/
static uintptr_t __main_stack[CONFIG_ESP_MAIN_TASK_STACK_SIZE / sizeof(uintptr_t)];
static StaticTask_t __main_task;

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

uint64_t systimer_ticks_to_us(uint64_t ticks)
{
    return ticks * 1000000 / CLK_TREE_systimer_freq();
}

uint64_t systimer_us_to_ticks(uint64_t us)
{
    return us * CLK_TREE_systimer_freq() / 1000000;
}

/****************************************************************************
 *  @implements: freertos main task
*****************************************************************************/
static void __esp_freertos_main_thread(void *arg)
{
    ARG_UNUSED(arg);
    __esp_rtos_initialize();

    extern __attribute__((noreturn)) void main(void);
    // TODO: process main() exit code
    main();

    // main should not return
    vTaskDelete(NULL);
}

void esp_rtos_bootstrap(void)
{
    // Initialize the cross-core interrupt on CPU0
    esp_crosscore_int_init();

    /*
    BaseType_t res = xTaskCreatePinnedToCore(esp_main_thread_entry, "main",
        ESP_TASK_MAIN_STACK, NULL,
        configMAX_PRIORITIES, NULL, ESP_TASK_MAIN_CORE
    );
    assert(res == pdTRUE);
    (void)res;
    */

   if (0 == __get_CORE_ID())
   {
        // TODO: main task pined to core?
        // CONFIG_ESP_MAIN_TASK_AFFINITY

        xTaskCreateStatic(__esp_freertos_main_thread, "esp_freertos_main_thread",
            CONFIG_ESP_MAIN_TASK_STACK_SIZE, NULL, configMAX_PRIORITIES,
            (void *)__main_stack, &__main_task
        );

        vTaskStartScheduler();
        abort();
   }
   else
   {
        xPortStartScheduler();
        abort(); // Only get to here if FreeRTOS somehow very broken
   }
}

__attribute__((weak))
void __esp_rtos_initialize(void)
{
}

// --------------- CPU[1:N-1] App Startup ------------------

#if !CONFIG_FREERTOS_UNICORE
void esp_startup_start_app_other_cores(void)
{
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
