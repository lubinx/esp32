#pragma once

#include <features.h>
#include <stdint.h>
#include <stdbool.h>

#include "clk_tree_defs.h"

__BEGIN_DECLS
/****************************************************************************
 * clk initialization using sdkconfig, calling by SystemInit()
 ****************************************************************************/
extern __attribute__((nothrow))
    void clk_tree_initialize(void);

/****************************************************************************
 * rc / external clocks, and pll configure
 ****************************************************************************/
extern __attribute__((nothrow, const))
    uint64_t clk_tree_sclk_freq(soc_sclk_t sclk);

    /**
     *  pll configure
    */
extern __attribute__((nothrow))
    int clk_tree_pll_conf(soc_pll_freq_sel_t sel);

    /**
     *  xtal ==> pll clock'frequency
    */
extern __attribute__((nothrow, const))
    uint64_t clk_tree_pll_freq(void);

/****************************************************************************
 * cpu / systick / ahb / apb
 ****************************************************************************/
    /**
     *  configure cpu clock route
     *  @returns
     *      0 if successful
     *      EINVAL
    */
extern __attribute__((nothrow))
    int clk_tree_cpu_conf(soc_cpu_sclk_sel_t sel, uint32_t div);

    /**
     *  cpu clock'frequency
    */
extern __attribute__((nothrow, const))
    uint64_t clk_tree_cpu_freq(void);

    /**
     *  configure systimer(systick) clock route
    */
extern __attribute__((nothrow))
    int clk_tree_systick_conf(soc_systick_sclk_sel_t sel, uint32_t div);

    /**
     *  systick clock'frequency
     *      this clock commonly used by rtos to provide thread/time/clock precision
    */
extern __attribute__((nothrow, const))
    uint64_t clk_tree_systick_freq(void);

    /**
     *  ahb / apb clock'frequency
    */
extern __attribute__((nothrow, const))
    uint64_t clk_tree_ahb_freq(void);
extern __attribute__((nothrow, const))
    uint64_t clk_tree_apb_freq(void);

/****************************************************************************
 *  rtc fast/slow clks
 *      TODO: move to RTC driver
 ****************************************************************************/
    /**
     *  configure rtc route
     *  @returns
     *      0 if successful
     *      EINVAL
    */
extern __attribute__((nothrow))
    int clk_tree_rtc_fast_conf(soc_rtc_fast_sclk_sel_t sel, uint32_t div);
extern __attribute__((nothrow))
    int clk_tree_rtc_slow_conf(soc_rtc_slow_sclk_sel_t sel, uint32_t div);

    /**
     *  rtc fast source clock'frequency
     *  @return 0 if its not exists
    */
extern __attribute__((nothrow, const))
    uint64_t clk_tree_rtc_fast_freq(void);
extern __attribute__((nothrow, const))
    uint64_t clk_tree_rtc_slow_freq(void);

/****************************************************************************
 *  clk gate, NOTE: obsolte periph_ctrl.h
 ****************************************************************************/
extern __attribute__((nothrow))
    void clk_tree_module_enable(periph_module_t periph);
extern __attribute__((nothrow))
    void clk_tree_module_disable(periph_module_t periph);
extern __attribute__((nothrow))
    void clk_tree_module_reset(periph_module_t periph);

extern __attribute__((nothrow, const))
    bool clk_tree_module_is_enable(periph_module_t periph);

/****************************************************************************
 *  esp-idf
 *      deprecated for compatible
 ****************************************************************************/
static inline
    void periph_module_enable(periph_module_t periph)
    {
        clk_tree_module_enable(periph);
    }

static inline
    void periph_module_disable(periph_module_t periph)
    {
        clk_tree_module_disable(periph);
    }

static inline
    void wifi_module_enable(void)
    {
        clk_tree_module_enable(PERIPH_WIFI_MODULE);
    }

static inline
    void wifi_module_disable(void)
    {
        clk_tree_module_disable(PERIPH_WIFI_MODULE);
    }

static inline
    void wifi_bt_common_module_enable(void)
    {
        clk_tree_module_enable(PERIPH_BT_MODULE);
    }

static inline
    void wifi_bt_common_module_disable(void)
    {
        clk_tree_module_disable(PERIPH_BT_MODULE);
    }

    /*
    enum clk_tree_src_freq_precision_t
    {
        CLK_TREE_SRC_FREQ_PRECISION_CACHED,   // Get value from the data cached by the driver; If the data is 0, then a calibration will be performed
        CLK_TREE_SRC_FREQ_PRECISION_APPROX,   // Get its approxiamte frequency value
        CLK_TREE_SRC_FREQ_PRECISION_EXACT,    // Always perform a calibration
        CLK_TREE_SRC_FREQ_PRECISION_INVALID,  // Invalid degree of precision
    };
    typedef enum clk_tree_src_freq_precision_t clk_tree_src_freq_precision_t;

extern __attribute__((nothrow, nonnull, const))
    esp_err_t clk_tree_src_get_freq_hz(soc_module_clk_t clk_src, enum clk_tree_src_freq_precision_t precision, uint64_t *freq_value);
    */

__END_DECLS
