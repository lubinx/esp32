#ifndef __ESP32S3_CLKTREE_DEFS_H
#define __ESP32S3_CLKTREE_DEFS_H        1

/*
soo..the esp-idf manual is not well documented
    this the real route we enabled in "CLK_initialize()"
    more clocks selection see "SUMMARY"

+----------+            +--------------+
| XTAL HF  +-----+----->| PLL 320/480M |
+----------+     |      +---+----------+
    40M          |          |      +----------+                     +--\
                 |          +----->| PLL 240M +-------------------->|   \
                 |          |      +----------+                     |   |
                 |          |      +----------+                     |   |
                 |          +----->| PLL 160M |----=--------------->| M |                   +-----------+
                 |          |      +----------+                     | U |------------------>| CPU       |
                 |          |      +----------+                     | X |                   +-----------+
                 |          +----->| PLL 80M  +---------+---------->|   |
                 |                 +----------+         |           |   |
                 +--------->+-------+                   |           |   |
+----------+     |          | DIV X +-------------------#----+----->|   /
| RC FAST  +-----#----+---->+-------+                   |    |      +--/
+----------+     |    |                                 |    |
    20M          |    |                                 |    |      +--\
                 |    |                                 |    +----->| M \                   +-----------+
                 |    |                                 |           | U |----+------------->| AHB / APB |
                 |    |                                 +---------->| X /    |              +-----------+
                 |    |                                             +--/     |
                 |    |                                                      |    +--\
                 |    |                                                      +--->|   \
                 |    |                                                           | M |     +-----------+
                 |    +---------------------------------------------------------->| U |---->| UART X    |
                 |    |                                                           | X |     +-----------+
                 +----#---------------------------------------------------------->|   /
                 |    |                                                           +--/
                 |    |
                 |    |                                                           +--\
                 |    +---------------------------------------------------------->| M \     +----------+
                 |    |     +---------+                                           | U |---->| RTC chip |
                 +----#---->| DIV 2   +------------------------------------------>| X /     +----------+
                      |     +---------+                                           +--/
                      |
                      |     +---------+                                           +--\
                      +---->| DIV 256 +------------------------------------------>| M \     +----------+
                            +---------+                                           | U |---->| RTC      |
+---------+                                +------------------------------------->| X /     +----------+
+ XTAL32K +---------------->+---------+    |                                      +--/
+---------+                 | DIV X   +----+
    32K        +------------+---------+
               |
+---------+    |
| RC SLOW +----+
+---------+
    136K

. RC FAST is optional powered, default is power off
. AHB/APB is highly depended by CPU clock, is a fixed "PLL 80M" when CPU is clocked by PLL, otherwise is equal to CPU clock
. RTChip is RTC FAST, its kind another *CHIP* can be programmed at running time

. in SUMMARY, ref table 7.4
----------------------------------------------------------------------------------------------------------------
            XTAL_CLK    APB_CLK    PLL_F160M_CLK    PLL_D2_CLK    RC_FAST_CLK    CRYPTO_PWM_CLK    LEDC_CLK
----------------------------------------------------------------------------------------------------------------
    CRYPTO                                                                              ✔️
----------------------------------------------------------------------------------------------------------------
    eFuse       ✔️      ✔️
----------------------------------------------------------------------------------------------------------------
    I2C         ✔️                                                  ✔️
----------------------------------------------------------------------------------------------------------------
    I2S         ✔️                      ✔️              ✔️
----------------------------------------------------------------------------------------------------------------
    LEDC        ✔️      ✔️                                          ✔️                              ✔️
----------------------------------------------------------------------------------------------------------------
    PCNT                ✔️
----------------------------------------------------------------------------------------------------------------
    PWM                                                                                 ✔️
----------------------------------------------------------------------------------------------------------------
    RMT         ✔️      ✔️                                          ✔️
----------------------------------------------------------------------------------------------------------------
    SARADC              ✔️                              ✔️
----------------------------------------------------------------------------------------------------------------
    SPI         ✔️      ✔️
----------------------------------------------------------------------------------------------------------------
    TIMG        ✔️      ✔️
----------------------------------------------------------------------------------------------------------------
    TWAI                ✔️
----------------------------------------------------------------------------------------------------------------
    UART        ✔️      ✔️                                          ✔️
----------------------------------------------------------------------------------------------------------------
    UHCI                ✔️
----------------------------------------------------------------------------------------------------------------
    USB                 ✔️
----------------------------------------------------------------------------------------------------------------
    SDIO        ✔️                      ✔️
----------------------------------------------------------------------------------------------------------------
    SYSTIMER    ✔️      ✔️
----------------------------------------------------------------------------------------------------------------
    LCD_CAM     ✔️                      ✔️              ✔️
----------------------------------------------------------------------------------------------------------------
*/

#include "soc/periph_defs.h"
    // alias
    typedef periph_module_t         PERIPH_module_t;

// provided source clocks, its XTAL/RC(fast/slow)
    #define XTAL_32M                    (32000000U)
    #define XTAL_40M                    (40000000U)
    #define XTAL32K_FREQ                (32768U)
    #define RC_FAST_FREQ                (20000000U)         // SOO..RC_FAST_FREQ is 20M, not documented 17.5M
    #define RC_SLOW_FREQ                (136000U)

// fixed divider
    #define XTAL_D2_FREQ                (XTAL_FREQ / 2)
    #define RC_FAST_D256_FREQ           (RC_FAST_FREQ / 256)

// this is tested minimal CPU working freq is 4M, any value smaller than this halt the CPU
    #define MINIAL_CPU_WORK_FREQ        (4000000U)

    enum PLL_freq_sel_t
    {
        PLL_FREQ_SEL_320M           = 0,
        PLL_FREQ_SEL_480M
    };

    enum CPU_sclk_sel_t
    {
        CPU_SCLK_SEL_XTAL           = 0,
        CPU_SCLK_SEL_PLL,
        CPU_SCLK_SEL_RC_FAST
    };

    enum SYSTIMER_sclk_sel_t
    {
        SYSTIMER_SCLK_SEL_XTAL      = 0,
        // SYSTIMER_SCLK_SEL_APB    <== no selectable? fixed at 16M
    };

    enum RTC_sclk_sel_t
    {
        RTC_SCLK_SEL_RC_SLOW        = 0,
        RTC_SCLK_SEL_XTAL32K,
        RTC_SLOW_SCLK_SEL_RC_FAST_D256,
    };

    enum RTC_FAST_sclk_sel_t
    {
        RTC_FAST_SCLK_SEL_XTAL_D2   = 0,
        RTC_FAST_SCLK_SEL_RC_FAST,
        // alias
        RTC_FAST_sCLK_SEL_XTAL_DIV = RTC_FAST_SCLK_SEL_XTAL_D2
    };

    enum UART_sclk_sel_t
    {
        UART_SCLK_SEL_APB           = 1,
        UART_SCLK_SEL_RC_FAST,
        UART_SCLK_SEL_XTAL
    };

    enum I2C_sclk_sel_t
    {
        I2C_SCLK_SEL_XTAL           = 0,
        I2C_SCLK_SEL_RC_FAST
    };

    enum I2S_sclk_sel_t
    {
        I2S_SCLK_SEL_XTAL           = 0,
        I2S_SCLK_SEL_PLL_D2,
        I2S_SCLK_SEL_PLL_F160M
    };

#endif
