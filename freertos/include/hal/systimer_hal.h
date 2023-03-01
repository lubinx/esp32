/**
 *  freertos minimal required SYSTIMER
*/
#pragma once

#include <sys/cdefs.h>
#include <stdint.h>

__BEGIN_DECLS
    typedef uint64_t (*ticks_to_us_func_t)(uint64_t ticks);
    typedef uint64_t (*us_to_ticks_func_t)(uint64_t us);

    struct systimer_hal_context_t
    {
        void *dev;      // struct systimer_dev_t * => void *, bcuz systimer_dev_t now is really nowhere to known
        ticks_to_us_func_t ticks_to_us;
        us_to_ticks_func_t us_to_ticks;
    };
    typedef struct systimer_hal_context_t systimer_hal_context_t;

    struct systimer_hal_tick_rate_ops_t
    {
        ticks_to_us_func_t ticks_to_us;
        us_to_ticks_func_t us_to_ticks;
    };
    typedef struct systimer_hal_tick_rate_ops_t systimer_hal_tick_rate_ops_t;

    enum systimer_alarm_mode_t
    {
        SYSTIMER_ALARM_MODE_ONESHOT,
        SYSTIMER_ALARM_MODE_PERIOD
    };
    typedef enum systimer_alarm_mode_t  systimer_alarm_mode_t;

extern __attribute__((nothrow))
    void systimer_hal_init(systimer_hal_context_t *hal);

static inline
    void systimer_hal_set_tick_rate_ops(systimer_hal_context_t *hal, systimer_hal_tick_rate_ops_t *ops)
    {
        hal->ticks_to_us = ops->ticks_to_us;
        hal->us_to_ticks = ops->us_to_ticks;
    }

extern __attribute__((nothrow))
    void systimer_hal_enable_counter(systimer_hal_context_t *hal, uint32_t counter_id);

extern __attribute__((nothrow))
    uint64_t systimer_hal_get_counter_value(systimer_hal_context_t *hal, uint32_t counter_id);

extern __attribute__((nothrow))
    void systimer_hal_set_alarm_period(systimer_hal_context_t *hal, uint32_t alarm_id, uint32_t period);

extern __attribute__((nothrow))
    void systimer_hal_enable_alarm_int(systimer_hal_context_t *hal, uint32_t alarm_id);

extern __attribute__((nothrow))
    void systimer_hal_select_alarm_mode(systimer_hal_context_t *hal, uint32_t alarm_id, systimer_alarm_mode_t mode);

extern __attribute__((nothrow))
    void systimer_hal_counter_value_advance(systimer_hal_context_t *hal, uint32_t counter_id, int64_t time_us);

extern __attribute__((nothrow))
    void systimer_hal_connect_alarm_counter(systimer_hal_context_t *hal, uint32_t alarm_id, uint32_t counter_id);

    /// debugger
extern __attribute__((nothrow))
    void systimer_hal_counter_can_stall_by_cpu(systimer_hal_context_t *hal, uint32_t counter_id, uint32_t cpu_id, bool can);

__END_DECLS
