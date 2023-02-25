#ifndef __ESP32S3_CLKTREE_DEFS_H
#define __ESP32S3_CLKTREE_DEFS_H        1

#include "soc/periph_defs.h"
    // alias
    typedef periph_module_t         PERIPH_module_t;

    #define CLK_TREE_XTAL_FREQ          (40000000)
    #define CLK_TREE_XTAL32K_FREQ       (32768)
    #define CLK_TREE_RC_FAST_FREQ       (17500000)
    #define CLK_TREE_RC_SLOW_FREQ       (136000)

    #define CLK_TREE_RC_FAST_D256       (CLK_TREE_RC_FAST_FREQ / 256)

    enum PLL_freq_sel_t
    {
        PLL_FREQ_SEL_320M           = 0,
        PLL_FREQ_SEL_480M
    };
    typedef enum PLL_freq_sel_t     PLL_freq_sel_t;

    enum CPU_sclk_sel_t
    {
        CPU_SCLK_SEL_XTAL           = 0,
        CPU_SCLK_SEL_PLL,
        CPU_SCLK_SEL_RC_FAST
    };
    typedef enum CPU_sclk_sel_t     CPU_sclk_sel_t;

    enum SYSTIMER_sclk_sel_t
    {
        SYSTIMER_SCLK_SEL_XTAL      = 0
    };
    typedef enum SYSTIMER_sclk_sel_t    SYSTIMER_sclk_sel_t;

    enum RTC_sclk_sel_t
    {
        RTC_SCLK_SEL_RC_SLOW        = 0,
        RTC_SCLK_SEL_XTAL,
        RTC_SLOW_SCLK_SEL_RC_FAST_D256,
    };
    typedef enum RTC_sclk_sel_t     RTC_sclk_sel_t;

    enum RTC_FAST_sclk_sel_t
    {
        RTC_FAST_SCLK_SEL_XTAL_D2 = 0,
        RTC_FAST_SCLK_SEL_RC_FAST
    };
    typedef enum RTC_FAST_sclk_sel_t    RTC_FAST_sclk_sel_t;

#endif
