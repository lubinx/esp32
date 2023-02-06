#include "soc/clk_tree_defs.h"

// #include "soc/system_struct.h"       external SYSTEM is not mapped in esp32s3.peripherals.ld
#include "soc/system_reg.h"
#include "soc/rtc_cntl_struct.h"

#include "clk_tree.h"

/****************************************************************************
 * @def
 ****************************************************************************/
#define CLK_LL_PLL_80M_FREQ             (80000000ULL)
#define CLK_LL_PLL_160M_FREQ            (160000000ULL)
#define CLK_LL_PLL_240M_FREQ            (240000000ULL)
#define CLK_LL_PLL_320M_FREQ            (320000000ULL)
#define CLK_LL_PLL_480M_FREQ            (480000000ULL)
#define CLK_LL_AHB_MAX_FREQ             CLK_LL_PLL_80M_FREQ

uint64_t systimer_ticks_to_us(uint64_t ticks)
{
    return ticks / 16;
}

uint64_t systimer_us_to_ticks(uint64_t us)
{
    return us * 16;
}

/****************************************************************************
 *  @implements: hw external clocks
 ****************************************************************************/
uint64_t clk_tree_rc_fast_freq(void)
{
    return SOC_CLK_RC_FAST_FREQ_APPROX;
}

uint64_t clk_tree_rc_slow_freq(void)
{
    return SOC_CLK_RC_SLOW_FREQ_APPROX;
}

uint64_t clk_tree_xtal_freq(void)
{
    return 40000000;

    /**
     *  REVIEW: clk_ll_xtal_load_freq_mhz() load from RTC area
     *      don't know why they doing that.
     *      basicly xtal is depended by chip, which is fixed value
    */
    /*
    uint32_t mhz = clk_ll_xtal_load_freq_mhz();
    if (0 == mhz)
    {
        ESP_LOGW(TAG, "invalid RTC_XTAL_FREQ_REG value, assume %dMHz", CONFIG_XTAL_FREQ);
        return CONFIG_XTAL_FREQ;
    }
    return mhz;
    */
}

uint64_t clk_tree_xtal32k_freq(void)
{
    return SOC_CLK_XTAL32K_FREQ_APPROX;
}

uint64_t clk_tree_pll_freq(void)
{
    // SYSTEM.cpu_per_conf.pll_freq_sel
    switch (REG_GET_FIELD(SYSTEM_CPU_PER_CONF_REG, SYSTEM_PLL_FREQ_SEL))
    {
    default:
        return 0;
    case 0:
        return CLK_LL_PLL_320M_FREQ;
    case 1:
        return CLK_LL_PLL_480M_FREQ;
    }
}

/****************************************************************************
 * cpu / systick / ahb / apb
 ****************************************************************************/
int clk_tree_cpu_route(soc_cpu_clk_src_t route)
{
    return ENOSYS;
}

static uint32_t clk_tree_cpu_divider(void)
{
    return REG_GET_FIELD(SYSTEM_SYSCLK_CONF_REG, SYSTEM_PRE_DIV_CNT) + 1;
}

uint64_t clk_tree_cpu_freq(void)
{
    switch ((soc_cpu_clk_src_t)REG_GET_FIELD(SYSTEM_SYSCLK_CONF_REG, SYSTEM_SOC_CLK_SEL))
    {
    //--- 40M -
    case SOC_CPU_CLK_SRC_XTAL:
        return clk_tree_xtal_freq() / clk_tree_cpu_divider();
    case SOC_CPU_CLK_SRC_PLL:
        switch (REG_GET_FIELD(SYSTEM_CPU_PER_CONF_REG, SYSTEM_CPUPERIOD_SEL))
        {
    // --- 80M
        case 0:
            return CLK_LL_PLL_80M_FREQ;
    // --- 160M
        case 1:
            return CLK_LL_PLL_160M_FREQ;
        case 2:
    // --- 240M
            // When PLL frequency selection is 320MHz but CPU frequency selection is 240MHz, it is an undetermined state.
            // It is checked in the upper layer.
            return CLK_LL_PLL_240M_FREQ;
        default:
            return 0;
        }

    // --- should be power up default
    case SOC_CPU_CLK_SRC_RC_FAST:
        return SOC_CLK_RC_FAST_FREQ_APPROX / clk_tree_cpu_divider();
    default:
        return 0;
    }
}

uint64_t clk_tree_systick_freq(void)
{
    // fixed at 16 Mhz
    return 16000000;
}

uint64_t clk_tree_ahb_freq(void)
{
    // AHB_CLK path is highly dependent on CPU_CLK path
    switch ((soc_cpu_clk_src_t)REG_GET_FIELD(SYSTEM_SYSCLK_CONF_REG, SYSTEM_SOC_CLK_SEL))
    {
    // --- equal to cpu
    case SOC_CPU_CLK_SRC_XTAL:
        return clk_tree_xtal_freq() / clk_tree_cpu_divider();
    case SOC_CPU_CLK_SRC_RC_FAST:
        return SOC_CLK_RC_FAST_FREQ_APPROX / clk_tree_cpu_divider();
    // ---

    case SOC_CPU_CLK_SRC_PLL:
        // AHB_CLK is a fixed value when CPU_CLK is clocked from PLL
        return CLK_LL_AHB_MAX_FREQ;
    }
}

// esp32s3 ahb = apb
uint64_t clk_tree_apb_freq(void)
    __attribute__((alias(("clk_tree_ahb_freq"))));

/****************************************************************************
 *  rtc fast/slow clks
 ****************************************************************************/
int clk_tree_rtc_fast_route(soc_rtc_fast_clk_src_t route)
{
    return ENOSYS;
}

int clk_tree_rtc_slow_route(soc_rtc_slow_clk_src_t route)
{
    return ENOSYS;
}

uint64_t clk_tree_rtc_fast_src_freq(void)
{
    return 0; // clk_tree_lp_fast_get_freq_hz(precision);
}

uint64_t clk_tree_rtc_slow_src_freq(void)
{
    switch ((soc_rtc_slow_clk_src_t)RTCCNTL.clk_conf.ana_clk_rtc_sel) // REG_GET_FIELD(RTC_CNTL_CLK_CONF_REG, RTC_CNTL_ANA_CLK_RTC_SEL)
    {
    case SOC_RTC_SLOW_CLK_SRC_RC_SLOW:
        return SOC_CLK_RC_SLOW_FREQ_APPROX;
    case SOC_RTC_SLOW_CLK_SRC_XTAL32K:
        return SOC_CLK_XTAL32K_FREQ_APPROX;
    case SOC_RTC_SLOW_CLK_SRC_RC_FAST_D256:
        return SOC_CLK_RC_FAST_D256_FREQ_APPROX;
    default:
        return 0;
    }
}
