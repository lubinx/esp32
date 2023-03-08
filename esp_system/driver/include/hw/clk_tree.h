#ifndef __HW_CLK_H
#define __HW_CLK_H                 1

#include <features.h>
#include <stdint.h>
#include <stdbool.h>

#include "clk_tree_defs.h"

__BEGIN_DECLS
/****************************************************************************
 *  initialization
 ****************************************************************************/
    // init
extern __attribute__((nothrow))
    void CLK_initialize(void);

/****************************************************************************
 *  CPU / main peripherals configure & frequencys
 ****************************************************************************/
    // PLL
extern __attribute__((nothrow))
    int CLK_pll_conf(enum PLL_freq_sel_t sel);
extern __attribute__((nothrow, const))
    uint64_t CLK_pll_freq(void);

    // CPU
extern __attribute__((nothrow))
    int CLK_cpu_conf(enum CPU_sclk_sel_t sel, uint32_t div);
extern __attribute__((nothrow, const))
    uint64_t CLK_cpu_freq(void);

    // SysTimer
extern __attribute__((nothrow))
    int CLK_systimer_conf(enum SYSTIMER_sclk_sel_t sel);
extern __attribute__((nothrow, const))
    uint64_t CLK_systimer_freq(void);

    // AHB/APB
extern __attribute__((nothrow, const))
    uint64_t CLK_ahb_freq(void);
extern __attribute__((nothrow, const))
    uint64_t CLK_apb_freq(void);

    // RTC
extern __attribute__((nothrow))
    int CLK_rtc_conf(enum RTC_sclk_sel_t sel);
extern __attribute__((nothrow, const))
    uint64_t CLK_rtc_freq(void);

/****************************************************************************
 *  peripheral module gating control
 ****************************************************************************/
extern __attribute__((nothrow, const))
    bool CLK_periph_is_enabled(PERIPH_module_t periph);
extern __attribute__((nothrow))
    void CLK_periph_enable(PERIPH_module_t periph);
extern __attribute__((nothrow))
    void CLK_periph_disable(PERIPH_module_t periph);
extern __attribute__((nothrow))
    void CLK_periph_reset(PERIPH_module_t periph);

/****************************************************************************
 *  esp-idf
 *      deprecated for compatible
 ****************************************************************************/
static inline
    void periph_module_enable(PERIPH_module_t periph)
    {
        CLK_periph_enable(periph);
    }

static inline
    void periph_module_disable(PERIPH_module_t periph)
    {
        CLK_periph_disable(periph);
    }

static inline
    void wifi_module_enable(void)
    {
        CLK_periph_enable(PERIPH_WIFI_MODULE);
    }

static inline
    void wifi_module_disable(void)
    {
        CLK_periph_disable(PERIPH_WIFI_MODULE);
    }

static inline
    void wifi_bt_common_module_enable(void)
    {
        CLK_periph_enable(PERIPH_BT_MODULE);
    }

static inline
    void wifi_bt_common_module_disable(void)
    {
        CLK_periph_disable(PERIPH_BT_MODULE);
    }

__END_DECLS
#endif
