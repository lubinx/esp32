/*
 * SPDX-FileCopyrightText: 2023 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>

#include "soc/soc_caps.h"
#include "soc/clk_tree_defs.h"
#include "soc/rtc.h"

#include "clk_tree.h"
#include "esp_err.h"
#include "esp_check.h"
#include "soc/rtc.h"

#include "hal/clk_tree_hal.h"
#include "hal/clk_tree_ll.h"

#include "esp_private/esp_clk.h"
#include "sdkconfig.h"


static const char *TAG = "clk_tree";

/****************************************************************************
 *  @internal
 ****************************************************************************/
#if SOC_CLK_RC_FAST_D256_SUPPORTED
    static uint32_t clk_tree_rc_fast_d256_get_freq_hz(clk_tree_src_freq_precision_t precision);
#endif

#if SOC_CLK_XTAL32K_SUPPORTED
    static uint32_t clk_tree_xtal32k_get_freq_hz(clk_tree_src_freq_precision_t precision);
#endif

#if SOC_CLK_OSC_SLOW_SUPPORTED
    static uint32_t clk_tree_osc_slow_get_freq_hz(clk_tree_src_freq_precision_t precision);
#endif

static uint32_t clk_tree_rc_fast_get_freq_hz(clk_tree_src_freq_precision_t precision);
static uint32_t clk_tree_lp_slow_get_freq_hz(clk_tree_src_freq_precision_t precision);
static uint32_t clk_tree_lp_fast_get_freq_hz(clk_tree_src_freq_precision_t precision);

/****************************************************************************
 *  @implements
 ****************************************************************************/
uint32_t clk_tree_get_module_freq(soc_module_clk_t clk_src)
{

}

esp_err_t clk_tree_src_get_freq_hz(soc_module_clk_t clk_src, enum clk_tree_src_freq_precision_t precision, uint32_t *freq)
{
    uint32_t clk_src_freq = 0;
    switch (clk_src)
    {
    case SOC_MOD_CLK_CPU:
        clk_src_freq = clk_hal_cpu_get_freq_hz();
        break;
    case SOC_MOD_CLK_APB:
        clk_src_freq = clk_hal_apb_get_freq_hz();
        break;
    case SOC_MOD_CLK_XTAL:
        clk_src_freq = clk_hal_xtal_get_freq_mhz() * MHZ;
        break;
    case SOC_MOD_CLK_PLL_F80M:
        clk_src_freq = CLK_LL_PLL_80M_FREQ_MHZ * MHZ;
        break;
    case SOC_MOD_CLK_PLL_F160M:
        clk_src_freq = CLK_LL_PLL_160M_FREQ_MHZ * MHZ;
        break;
    case SOC_MOD_CLK_PLL_D2:
        clk_src_freq = (clk_ll_bbpll_get_freq_mhz() * MHZ) >> 1;
        break;
    case SOC_MOD_CLK_RTC_SLOW:
        clk_src_freq = clk_tree_lp_slow_get_freq_hz(precision);
        break;
    case SOC_MOD_CLK_RTC_FAST:
        clk_src_freq = clk_tree_lp_fast_get_freq_hz(precision);
        break;
    case SOC_MOD_CLK_RC_FAST:
    case SOC_MOD_CLK_TEMP_SENSOR:
        clk_src_freq = clk_tree_rc_fast_get_freq_hz(precision);
        break;
    case SOC_MOD_CLK_RC_FAST_D256:
        clk_src_freq = clk_tree_rc_fast_d256_get_freq_hz(precision);
        break;
    case SOC_MOD_CLK_XTAL32K:
        clk_src_freq = clk_tree_xtal32k_get_freq_hz(precision);
        break;

    default:
        clk_src_freq = 0;
        break;
    }

    if (clk_src_freq)
    {
        *freq = clk_src_freq;
        return ESP_OK;
    }
    else
    {
        ESP_LOGE(TAG, "freq shouldn't be 0, calibration failed");
        return ESP_FAIL;
    }
}


typedef struct clk_tree_calibrated_freq_t clk_tree_calibrated_freq_t;

struct clk_tree_calibrated_freq_t
{
    #if SOC_CLK_RC_FAST_D256_SUPPORTED
        uint32_t rc_fast_d256;
    #elif SOC_CLK_RC_FAST_SUPPORT_CALIBRATION // && !SOC_CLK_RC_FAST_D256_SUPPORTED
        uint32_t rc_fast;
    #endif
    #if SOC_CLK_XTAL32K_SUPPORTED
        uint32_t xtal32k;
    #endif
    #if SOC_CLK_OSC_SLOW_SUPPORTED
        uint32_t osc_slow;
    #endif
};

// TODO: Better to implement a spinlock for the static variables
static clk_tree_calibrated_freq_t s_calibrated_freq = {0};

/* Number of cycles for RTC_SLOW_CLK calibration */
#define RTC_SLOW_CLK_CAL_CYCLES     CONFIG_RTC_CLK_CAL_CYCLES
/* Number of cycles for ~32kHz clocks calibration (rc_fast_d256, xtal32k, osc_slow, rc32k) */
#define DEFAULT_32K_CLK_CAL_CYCLES  100
/* Number of cycles for RC_FAST calibration */
#define DEFAULT_RC_FAST_CAL_CYCLES  10000  // RC_FAST has a higher frequency, therefore, requires more cycles to get an accurate value


/**
 * Performs a frequency calibration to RTC slow clock
 *
 * slowclk_cycles Number of slow clock cycles to count.
 *                If slowclk_cycles = 0, calibration will not be performed. Clock's theoretical value will be used.
 *
 * Returns the number of XTAL clock cycles within the given number of slow clock cycles
 * It returns 0 if calibration failed, i.e. clock is not running
 */
static uint32_t clk_tree_rtc_slow_calibration(uint32_t slowclk_cycles)
{
    uint32_t cal_val = 0;
    if (slowclk_cycles > 0) {
        cal_val = rtc_clk_cal(RTC_CAL_RTC_MUX, slowclk_cycles);
    } else {
        const uint64_t cal_dividend = (1ULL << RTC_CLK_CAL_FRACT) * 1000000ULL;
        uint32_t source_approx_freq = clk_hal_lp_slow_get_freq_hz();
        assert(source_approx_freq);
        cal_val = (uint32_t)(cal_dividend / source_approx_freq);
    }
    if (cal_val) {
        ESP_EARLY_LOGD(TAG, "RTC_SLOW_CLK calibration value: %d", cal_val);
        // Update the calibration value of RTC_SLOW_CLK
        esp_clk_slowclk_cal_set(cal_val);
    }
    return cal_val;
}

#if SOC_CLK_RC_FAST_D256_SUPPORTED
uint32_t clk_tree_rc_fast_d256_get_freq_hz(clk_tree_src_freq_precision_t precision)
{
    switch (precision) {
    case CLK_TREE_SRC_FREQ_PRECISION_APPROX:
        return SOC_CLK_RC_FAST_D256_FREQ_APPROX;
    case CLK_TREE_SRC_FREQ_PRECISION_CACHED:
        if (!s_calibrated_freq.rc_fast_d256) {
            s_calibrated_freq.rc_fast_d256 = rtc_clk_freq_cal(rtc_clk_cal(RTC_CAL_8MD256, DEFAULT_32K_CLK_CAL_CYCLES));
        }
        return s_calibrated_freq.rc_fast_d256;
    case CLK_TREE_SRC_FREQ_PRECISION_EXACT:
        s_calibrated_freq.rc_fast_d256 = rtc_clk_freq_cal(rtc_clk_cal(RTC_CAL_8MD256, DEFAULT_32K_CLK_CAL_CYCLES));
        return s_calibrated_freq.rc_fast_d256;
    default:
        return 0;
    }
}
#endif

#if SOC_CLK_XTAL32K_SUPPORTED
uint32_t clk_tree_xtal32k_get_freq_hz(clk_tree_src_freq_precision_t precision)
{
    switch (precision) {
    case CLK_TREE_SRC_FREQ_PRECISION_APPROX:
        return SOC_CLK_XTAL32K_FREQ_APPROX;
    case CLK_TREE_SRC_FREQ_PRECISION_CACHED:
        if (!s_calibrated_freq.xtal32k) {
            s_calibrated_freq.xtal32k = rtc_clk_freq_cal(rtc_clk_cal(RTC_CAL_32K_XTAL, DEFAULT_32K_CLK_CAL_CYCLES));
        }
        return s_calibrated_freq.xtal32k;
    case CLK_TREE_SRC_FREQ_PRECISION_EXACT:
        s_calibrated_freq.xtal32k = rtc_clk_freq_cal(rtc_clk_cal(RTC_CAL_32K_XTAL, DEFAULT_32K_CLK_CAL_CYCLES));
        return s_calibrated_freq.xtal32k;
    default:
        return 0;
    }
}
#endif

#if SOC_CLK_OSC_SLOW_SUPPORTED
uint32_t clk_tree_osc_slow_get_freq_hz(clk_tree_src_freq_precision_t precision)
{
    switch (precision) {
    case CLK_TREE_SRC_FREQ_PRECISION_APPROX:
        return SOC_CLK_OSC_SLOW_FREQ_APPROX;
    case CLK_TREE_SRC_FREQ_PRECISION_CACHED:
        if (!s_calibrated_freq.osc_slow) {
            s_calibrated_freq.osc_slow = rtc_clk_freq_cal(rtc_clk_cal(RTC_CAL_32K_OSC_SLOW, DEFAULT_32K_CLK_CAL_CYCLES));
        }
        return s_calibrated_freq.osc_slow;
    case CLK_TREE_SRC_FREQ_PRECISION_EXACT:
        s_calibrated_freq.osc_slow = rtc_clk_freq_cal(rtc_clk_cal(RTC_CAL_32K_OSC_SLOW, DEFAULT_32K_CLK_CAL_CYCLES));
        return s_calibrated_freq.osc_slow;
    default:
        return 0;
    }
}
#endif

uint32_t clk_tree_lp_slow_get_freq_hz(clk_tree_src_freq_precision_t precision)
{
    switch (precision) {
    case CLK_TREE_SRC_FREQ_PRECISION_CACHED:
        // This returns calibrated (if CONFIG_xxx_RTC_CLK_CAL_CYCLES) value stored in RTC storage register
        return rtc_clk_freq_cal(clk_ll_rtc_slow_load_cal());
    case CLK_TREE_SRC_FREQ_PRECISION_APPROX:
        return clk_hal_lp_slow_get_freq_hz();
    case CLK_TREE_SRC_FREQ_PRECISION_EXACT:
        return rtc_clk_freq_cal(clk_tree_rtc_slow_calibration(RTC_SLOW_CLK_CAL_CYCLES));
    default:
        return 0;
    }
}

uint32_t clk_tree_rc_fast_get_freq_hz(clk_tree_src_freq_precision_t precision)
{
#if SOC_CLK_RC_FAST_SUPPORT_CALIBRATION
    if (precision == CLK_TREE_SRC_FREQ_PRECISION_APPROX) {
        return SOC_CLK_RC_FAST_FREQ_APPROX;
    }
#if SOC_CLK_RC_FAST_D256_SUPPORTED
    // If RC_FAST_D256 clock exists, calibration on a slow freq clock is much faster (less slow clock cycles need to wait)
    return clk_tree_rc_fast_d256_get_freq_hz(precision) << 8;
#else
    // Calibrate directly on the RC_FAST clock requires much more slow clock cycles to get an accurate freq value
    if (precision != CLK_TREE_SRC_FREQ_PRECISION_CACHED || !s_calibrated_freq.rc_fast) {
        s_calibrated_freq.rc_fast = rtc_clk_freq_cal(rtc_clk_cal(RTC_CAL_RC_FAST, DEFAULT_RC_FAST_CAL_CYCLES));
    }
    return s_calibrated_freq.rc_fast;
#endif //SOC_CLK_RC_FAST_D256_SUPPORTED
#else //!SOC_CLK_RC_FAST_SUPPORT_CALIBRATION
    if (precision != CLK_TREE_SRC_FREQ_PRECISION_APPROX) {
        // No way of getting exact rc_fast freq
        ESP_HW_LOGW(TAG, "unable to get the exact freq of rc_fast_clk, returning its approx. freq value");
    }
    return SOC_CLK_RC_FAST_FREQ_APPROX;
#endif //SOC_CLK_RC_FAST_SUPPORT_CALIBRATION
}

uint32_t clk_tree_lp_fast_get_freq_hz(clk_tree_src_freq_precision_t precision)
{
    switch (clk_ll_rtc_fast_get_src()) {
    case SOC_RTC_FAST_CLK_SRC_XTAL_DIV:
#if CONFIG_IDF_TARGET_ESP32 || CONFIG_IDF_TARGET_ESP32S2 //SOC_RTC_FAST_CLK_SRC_XTAL_D4
        return clk_hal_xtal_get_freq_mhz() * MHZ >> 2;
#else //SOC_RTC_FAST_CLK_SRC_XTAL_D2
        return clk_hal_xtal_get_freq_mhz() * MHZ >> 1;
#endif
    case SOC_RTC_FAST_CLK_SRC_RC_FAST:
        return clk_tree_rc_fast_get_freq_hz(precision) / clk_ll_rc_fast_get_divider();
    default:
        // Invalid clock source
        assert(false);
        return 0;
    }
}
