#ifndef __HW_CLK_TREE_H
#define __HW_CLK_TREE_H                 1

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
    void CLK_TREE_initialize(void);

/****************************************************************************
 *  main clock and configure
 ****************************************************************************/
    // PLL
extern __attribute__((nothrow))
    int CLK_TREE_pll_conf(PLL_freq_sel_t sel);
extern __attribute__((nothrow, const))
    uint64_t CLK_TREE_pll_freq(void);

    // CPU
extern __attribute__((nothrow))
    int CLK_TREE_cpu_conf(CPU_sclk_sel_t sel, uint32_t div);
extern __attribute__((nothrow, const))
    uint64_t CLK_TREE_cpu_freq(void);

    // SysTimer
extern __attribute__((nothrow))
    int CLK_TREE_systimer_conf(SYSTIMER_sclk_sel_t sel);
extern __attribute__((nothrow, const))
    uint64_t CLK_TREE_systimer_freq(void);

    // AHB/APB
extern __attribute__((nothrow, const))
    uint64_t CLK_TREE_ahb_freq(void);
extern __attribute__((nothrow, const))
    uint64_t CLK_TREE_apb_freq(void);

    // RTC
extern __attribute__((nothrow))
    int CLK_TREE_rtc_conf(RTC_sclk_sel_t sel);
extern __attribute__((nothrow, const))
    uint64_t CLK_TREE_rtc_sclk_freq(void);

/****************************************************************************
 *  peripheral module gating control
 ****************************************************************************/
extern __attribute__((nothrow, const))
    bool CLK_TREE_periph_is_enable(PERIPH_module_t periph);
extern __attribute__((nothrow))
    void CLK_TREE_periph_enable(PERIPH_module_t periph);
extern __attribute__((nothrow))
    void CLK_TREE_periph_disable(PERIPH_module_t periph);
extern __attribute__((nothrow))
    void CLK_TREE_periph_reset(PERIPH_module_t periph);

/****************************************************************************
 *  esp-idf
 *      deprecated for compatible
 ****************************************************************************/
static inline
    void periph_module_enable(PERIPH_module_t periph)
    {
        CLK_TREE_periph_enable(periph);
    }

static inline
    void periph_module_disable(PERIPH_module_t periph)
    {
        CLK_TREE_periph_disable(periph);
    }

static inline
    void wifi_module_enable(void)
    {
        CLK_TREE_periph_enable(PERIPH_WIFI_MODULE);
    }

static inline
    void wifi_module_disable(void)
    {
        CLK_TREE_periph_disable(PERIPH_WIFI_MODULE);
    }

static inline
    void wifi_bt_common_module_enable(void)
    {
        CLK_TREE_periph_enable(PERIPH_BT_MODULE);
    }

static inline
    void wifi_bt_common_module_disable(void)
    {
        CLK_TREE_periph_disable(PERIPH_BT_MODULE);
    }

__END_DECLS
#endif
