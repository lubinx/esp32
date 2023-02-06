#pragma once
#include <features.h>
#include <stdint.h>
#include <sys/errno.h>

#include "soc/clk_tree_defs.h"

__BEGIN_DECLS
/****************************************************************************
 *  hw rc / external clocks
 ****************************************************************************/
extern __attribute__((nothrow, const))
    uint64_t clk_tree_rc_fast_freq(void);
extern __attribute__((nothrow, const))
    uint64_t clk_tree_rc_slow_freq(void);


extern __attribute__((nothrow, const))
    uint64_t clk_tree_xtal_freq(void);

extern __attribute__((nothrow, const))
    uint64_t clk_tree_xtal32k_freq(void);

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
    int clk_tree_cpu_route(soc_cpu_clk_src_t route);

    /**
     *  cpu clock'frequency
    */
extern __attribute__((nothrow, const))
    uint64_t clk_tree_cpu_freq(void);

    /**
     *  systick clock'frequency
     *      this clock used by rtos to provide thread/time/clock precision
     *      TODO: somehow systick has  no route for it, it fixed by XTAL / 2.5 in esp32s3
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
 *  rtc clks
 ****************************************************************************/
    /**
     *  configure rtc route
     *  @returns
     *      0 if successful
     *      EINVAL
    */
extern __attribute__((nothrow))
    int clk_tree_rtc_fast_route(soc_rtc_fast_clk_src_t route);
extern __attribute__((nothrow))
    int clk_tree_rtc_slow_route(soc_rtc_slow_clk_src_t route);

    /**
     *  rtc fast source clock'frequency
     *  @return 0 if its not exists
    */
extern __attribute__((nothrow, const))
    uint64_t clk_tree_rtc_fast_src_freq(void);
extern __attribute__((nothrow, const))
    uint64_t clk_tree_rtc_slow_src_freq(void);

/****************************************************************************
 *  esp-idf
 *      TODO: deprecated: will remove later
 ****************************************************************************/
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
