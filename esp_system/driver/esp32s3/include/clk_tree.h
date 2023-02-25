#ifndef __ESP32S3_CLK_TREE_H
#define __ESP32S3_CLK_TREE_H            1

#include "clk_tree_defs.h"
#include "hw/clk_tree.h"

#include "soc/uart_struct.h"

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

    // fast RTC
extern __attribute__((nothrow, const))
    uint64_t CLK_TREE_fast_rtc_sclk_freq(void);
extern __attribute__((nothrow))
    int CLK_TREE_fast_rtc_conf(RTC_FAST_sclk_sel_t sel, uint32_t div);

    // UART
extern __attribute__((nothrow))
    int CLK_TREE_uart_conf(uart_dev_t *dev, UART_sclk_sel_t sel);
extern __attribute__((nothrow))
    uint64_t CLK_TREE_uart_sclk_freq(uart_dev_t *dev);

__END_DECLS
#endif
