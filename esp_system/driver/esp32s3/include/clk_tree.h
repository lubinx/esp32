#ifndef __ESP32S3_CLK_TREE_H
#define __ESP32S3_CLK_TREE_H            1

#include "clk_tree_defs.h"
#include "hw/clk_tree.h"
/*
+-------------+            +--------------+
| XTAL HF     +-----+----->| PLL 320/480M |
+-------------+     |      +----+---------+
    40M             |           |     +----------+                     +--\
                    |           +---->| PLL 80M  +--------------+----->|   \
                    |           |     +----------+              |      |   |
                    |           |     +----------+              |      |   |
                    |           +---->| PLL 160M |---------+----#----->| M |            +-----+
                    |           |     +----------+         |    |      | U |----------->| CPU |
                    |           |     +----------+         |    |      | X |            +-----+
                    |           +---->| PLL 240M +----+----#----#----->|   |
                    |                 +----------+    |    |    |      |   |
+-------------+     +-----+                           |    |    |      |   |
| INT FAST RC +---->| DIV +-----+---------------------#----#----#----->|   /
+-------------+     +-----+     |                     |    |    |      +--/
    17.5M                       |                     |    |    |
                                |                     |    |    |      +--\
                                |                     |    |    +----->| M \            +---------+
                                |                     |    |           | U |----------->| AHB/APB |
                                +---------------------#----#---------->| X /            +---------+
                                                      |    |           +--/
                                                      |    |
                                                      |    |
                                                      |    +--
                                                      |
                                                      +--

+-------------+
| XTAL 32K    |
+-------------+
    32K

+-------------+
| INT SLOW RC |
+-------------+
    136K

. AHB/APB is a fixed "PLL 80M" when CPU is clocked by PLL, otherwise is equal to CPU

*/

__BEGIN_DECLS

extern __attribute__((nothrow, const))
    uint64_t CLK_TREE_rtc_fast_freq(void);

extern __attribute__((nothrow))
    int CLK_TREE_rtc_fast_conf(RTC_FAST_sclk_sel_t sel, uint32_t div);

__END_DECLS
#endif
