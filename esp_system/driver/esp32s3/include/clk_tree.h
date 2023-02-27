#ifndef __ESP32S3_CLK_TREE_H
#define __ESP32S3_CLK_TREE_H            1

#include "clk_tree_defs.h"
#include "hw/clk_tree.h"

#include "soc/uart_struct.h"
#include "soc/i2c_struct.h"
#include "soc/i2s_struct.h"
#include "soc/spi_struct.h"

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

    // I2C
extern __attribute__((nothrow))
    int CLK_TREE_i2c_conf(i2c_dev_t *dev, I2C_sclk_sel_t sel);
extern __attribute__((nothrow))
    uint64_t CLK_TREE_i2c_sclk_freq(i2c_dev_t *dev);

    // I2S: rx
extern __attribute__((nothrow))
    int CLK_TREE_i2s_rx_conf(i2s_dev_t *dev, I2S_sclk_sel_t sel);
extern __attribute__((nothrow))
    uint64_t CLK_TREE_i2S_rx_sclk_freq(i2s_dev_t *dev);
    // I2S: tx
extern __attribute__((nothrow))
    int CLK_TREE_i2s_tx_conf(i2s_dev_t *dev, I2S_sclk_sel_t sel);
extern __attribute__((nothrow))
    uint64_t CLK_TREE_i2S_tx_sclk_freq(i2s_dev_t *dev);

__END_DECLS
#endif
