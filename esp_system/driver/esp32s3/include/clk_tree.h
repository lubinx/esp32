#ifndef __ESP32S3_CLK_TREE_H
#define __ESP32S3_CLK_TREE_H            1

#include "clk_tree_defs.h"
#include "hw/clk_tree.h"

#include "soc/uart_struct.h"

__BEGIN_DECLS

// internal RC FAST
extern __attribute__((nothrow))
    unsigned RC_FAST_SCLK_ref(void);
extern __attribute__((nothrow))
    unsigned RC_FAST_SCLK_release(void);

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
