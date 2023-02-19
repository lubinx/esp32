#ifndef __ESP32S3_CLKTREE_DEFS_H
#define __ESP32S3_CLKTREE_DEFS_H        1

#include "soc/clk_tree_defs.h"
#include "soc/periph_defs.h"

    /// provided clock source types, xtal, xtal32k, rc etc..
    enum soc_sclk_t
    {
        SOC_SCLK_INT_RC_FAST            = SOC_ROOT_CLK_INT_RC_FAST,
        SOC_SCLK_INT_RC_SLOW            = SOC_ROOT_CLK_INT_RC_SLOW,
        SOC_SCLK_XTAL                   = SOC_ROOT_CLK_EXT_XTAL,
        SOC_SCLK_XTAL32K                = SOC_ROOT_CLK_EXT_XTAL32K,
    };
    typedef enum soc_sclk_t             soc_sclk_t;

    #define SOC_PLL_320M_FREQ               (320000000ULL)
    #define SOC_PLL_480M_FREQ               (480000000ULL)

    enum soc_pll_freq_sel_t
    {
        SOC_PLL_320M                    = 0,
        SOC_PLL_480M
    };
    typedef enum soc_pll_freq_sel_t     soc_pll_freq_sel_t;

    typedef soc_cpu_clk_src_t           soc_cpu_sclk_sel_t;
    typedef soc_periph_systimer_clk_src_t   soc_systick_sclk_sel_t;

    typedef soc_rtc_fast_clk_src_t      soc_rtc_fast_sclk_sel_t;
    typedef soc_rtc_slow_clk_src_t      soc_rtc_slow_sclk_sel_t;

#endif
