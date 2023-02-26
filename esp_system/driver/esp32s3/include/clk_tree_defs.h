#ifndef __ESP32S3_CLKTREE_DEFS_H
#define __ESP32S3_CLKTREE_DEFS_H        1

/*
+----------+            +--------------+
| XTAL HF  +-----+----->| PLL 320/480M |
+----------+     |      +----+---------+
    40M          |           |     +----------+                     +--\
                 |           +---->| PLL 80M  +--------------+----->|   \
                 |           |     +----------+              |      |   |
                 |           |     +----------+              |      |   |
                 |           +---->| PLL 160M |---------+----#----->| M |                +-----+
                 |           |     +----------+         |    |      | U |--------------->| CPU |
                 |           |     +----------+         |    |      | X |                +-----+
                 |           +---->| PLL 240M +----+----#----#----->|   |
                 |                 +----------+    |    |    |      |   |
                 +---------------->+-----+         |    |    |      |   |
+----------+     |                 | DIV +----+----#----#----#----->|   /
| RC FAST  +-----#----+----------->+-----+    |    |    |    |      +--/
+----------+     |    |                       |    |    |    |
    20M          |    |                       |    |    |    |      +--\
                 |    |                       |    |    |    +----->| M \                +---------+
                 |    |                       |    |    |           | U |----+---------->| AHB/APB |
                 |    |                       +----#----#---------->| X /    |           +---------+
                 |    |                            |    |           +--/     |
                 |    |                            |    |                    |  +--\
                 |    |                            |    |                    +->| M \    +--------+
                 |    +----------------------------#----#---------------------->| U |--->| UART X |
                 +---------------------------------#----#---------------------->| X /    +--------+
                                                   |    |                       +--/
                                                   |    |
+---------+
| XTAL32K |
+---------+
    32K

+---------+
| RC SLOW |
+---------+
    136K

. AHB/APB is a fixed "PLL 80M" when CPU is clocked by PLL, otherwise is equal to CPU (XTAL)
*/

#include "soc/periph_defs.h"
    // alias
    typedef periph_module_t         PERIPH_module_t;

// provided source clocks
    #define XTAL_32M                    (32000000U)
    #define XTAL_40M                    (40000000U)
    #define XTAL32K_FREQ                (32768U)
    #define RC_FAST_FREQ                (20000000U)         // SOO..RC_FAST_FREQ is 20M, not documented 17.5M
    #define RC_SLOW_FREQ                (136000U)

    // this is tested minimal CPU working freq, maybe caused by freertos
    #define MINIAL_CPU_WORK_FREQ        (4000000U)

// fixed div
    #define CLK_TREE_XTAL_D2_FREQ       (XTAL_FREQ / 2)
    #define CLK_TREE_RC_FAST_D256_FREQ  (RC_FAST_FREQ / 256)

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
        RTC_FAST_SCLK_SEL_XTAL_D2   = 0,
        RTC_FAST_SCLK_SEL_RC_FAST
    };
    typedef enum RTC_FAST_sclk_sel_t    RTC_FAST_sclk_sel_t;

    enum UART_sclk_sel_t
    {
        UART_SCLK_SEL_APB           = 1,
        UART_SCLK_SEL_RC_FAST,
        UART_SCLK_SEL_XTAL
    };
    typedef enum UART_sclk_sel_t    UART_sclk_sel_t;

#endif
