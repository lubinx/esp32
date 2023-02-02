#pragma once
#include <features.h>
#include <stdint.h>
#include <sys/errno.h>

#include "soc/clk_tree_defs.h"

enum clk_tree_src_freq_precision_t
{
    CLK_TREE_SRC_FREQ_PRECISION_CACHED,   /*< Get value from the data cached by the driver; If the data is 0, then a calibration will be performed */
    CLK_TREE_SRC_FREQ_PRECISION_APPROX,   /*< Get its approxiamte frequency value */
    CLK_TREE_SRC_FREQ_PRECISION_EXACT,    /*< Always perform a calibration */
    CLK_TREE_SRC_FREQ_PRECISION_INVALID,  /*< Invalid degree of precision */
};
typedef enum clk_tree_src_freq_precision_t clk_tree_src_freq_precision_t;

__BEGIN_DECLS

extern __attribute__((nothrow))
    uint32_t clk_tree_get_module_freq(soc_module_clk_t clk_src);

extern __attribute__((nothrow, nonnull))
    esp_err_t clk_tree_src_get_freq_hz(soc_module_clk_t clk_src, enum clk_tree_src_freq_precision_t precision, uint32_t *freq_value);

__END_DECLS
