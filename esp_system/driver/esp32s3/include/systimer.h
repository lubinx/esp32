#ifndef __ESP32S3_SYSTIMER_H
#define __ESP32S3_SYSTIMER_H            1

#include <sys/cdefs.h>
#include "soc/systimer_struct.h"

__BEGIN_DECLS

/******************* Clock *************************/

static inline
    void systimer_ll_enable_clock(systimer_dev_t *dev, bool en)
    {
        dev->conf.clk_en = en;
    }

static inline
    void systimer_ll_set_clock_source(soc_periph_systimer_clk_src_t clk_src)
    {
        (void)clk_src;
    }

static inline
    soc_periph_systimer_clk_src_t systimer_ll_get_clock_source(void)
    {
        return SYSTIMER_CLK_SRC_XTAL;
    }

/******************* Counter *************************/

static inline
    void systimer_ll_enable_counter(systimer_dev_t *dev, uint32_t counter_id, bool en)
    {
        if (en) {
            dev->conf.val |= 1 << (30 - counter_id);
        } else {
            dev->conf.val &= ~(1 << (30 - counter_id));
        }
    }

static inline
    void systimer_ll_counter_can_stall_by_cpu(systimer_dev_t *dev, uint32_t counter_id, uint32_t cpu_id, bool can)
    {
        if (can) {
            dev->conf.val |= 1 << ((28 - counter_id * 2) - cpu_id);
        } else {
            dev->conf.val &= ~(1 << ((28 - counter_id * 2) - cpu_id));
        }
    }

static inline
    void systimer_ll_counter_snapshot(systimer_dev_t *dev, uint32_t counter_id)
    {
        dev->unit_op[counter_id].timer_unit_update = 1;
    }

static inline
    bool systimer_ll_is_counter_value_valid(systimer_dev_t *dev, uint32_t counter_id)
    {
        return dev->unit_op[counter_id].timer_unit_value_valid;
    }

static inline
    void systimer_ll_set_counter_value(systimer_dev_t *dev, uint32_t counter_id, uint64_t value)
    {
        dev->unit_load_val[counter_id].hi.timer_unit_load_hi = value >> 32;
        dev->unit_load_val[counter_id].lo.timer_unit_load_lo = value & 0xFFFFFFFF;
    }

static inline
    uint32_t systimer_ll_get_counter_value_low(systimer_dev_t *dev, uint32_t counter_id)
    {
        return dev->unit_val[counter_id].lo.timer_unit_value_lo;
    }

static inline
    uint32_t systimer_ll_get_counter_value_high(systimer_dev_t *dev, uint32_t counter_id)
    {
        return dev->unit_val[counter_id].hi.timer_unit_value_hi;
    }

static inline
    void systimer_ll_apply_counter_value(systimer_dev_t *dev, uint32_t counter_id)
    {
        dev->unit_load[counter_id].val = 0x01;
    }

static inline
    void systimer_ll_set_alarm_target(systimer_dev_t *dev, uint32_t alarm_id, uint64_t value)
    {
        dev->target_val[alarm_id].hi.timer_target_hi = value >> 32;
        dev->target_val[alarm_id].lo.timer_target_lo = value & 0xFFFFFFFF;
    }

static inline
    uint64_t systimer_ll_get_alarm_target(systimer_dev_t *dev, uint32_t alarm_id)
    {
        return ((uint64_t)(dev->target_val[alarm_id].hi.timer_target_hi) << 32) | dev->target_val[alarm_id].lo.timer_target_lo;
    }

static inline
    void systimer_ll_connect_alarm_counter(systimer_dev_t *dev, uint32_t alarm_id, uint32_t counter_id)
    {
        dev->target_conf[alarm_id].target_timer_unit_sel = counter_id;
    }

static inline
    void systimer_ll_enable_alarm_oneshot(systimer_dev_t *dev, uint32_t alarm_id)
    {
        dev->target_conf[alarm_id].target_period_mode = 0;
    }

static inline
    void systimer_ll_enable_alarm_period(systimer_dev_t *dev, uint32_t alarm_id)
    {
        dev->target_conf[alarm_id].target_period_mode = 1;
    }

static inline
    void systimer_ll_set_alarm_period(systimer_dev_t *dev, uint32_t alarm_id, uint32_t period)
    {
        dev->target_conf[alarm_id].target_period = period;
    }

static inline
    uint32_t systimer_ll_get_alarm_period(systimer_dev_t *dev, uint32_t alarm_id)
    {
        return dev->target_conf[alarm_id].target_period;
    }

static inline
    void systimer_ll_apply_alarm_value(systimer_dev_t *dev, uint32_t alarm_id)
    {
        dev->comp_load[alarm_id].val = 0x01;
    }

static inline
    void systimer_ll_enable_alarm(systimer_dev_t *dev, uint32_t alarm_id, bool en)
    {
        if (en) {
            dev->conf.val |= 1 << (24 - alarm_id);
        } else {
            dev->conf.val &= ~(1 << (24 - alarm_id));
        }
    }

/******************* Interrupt *************************/

static inline
    void systimer_ll_enable_alarm_int(systimer_dev_t *dev, uint32_t alarm_id, bool en)
    {
        if (en) {
            dev->int_ena.val |= 1 << alarm_id;
        } else {
            dev->int_ena.val &= ~(1 << alarm_id);
        }
    }

static inline
    bool systimer_ll_is_alarm_int_fired(systimer_dev_t *dev, uint32_t alarm_id)
    {
        return dev->int_st.val & (1 << alarm_id);
    }

static inline
    void systimer_ll_clear_alarm_int(systimer_dev_t *dev, uint32_t alarm_id)
    {
        dev->int_clr.val |= 1 << alarm_id;
    }

__END_DECLS
#endif
