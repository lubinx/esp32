#ifndef __ESP32S3_CLK_H
#define __ESP32S3_CLK_H            1

#include "clk_tree_defs.h"
#include "hw/clk_tree.h"

#include "soc/uart_struct.h"
#include "soc/i2c_struct.h"
#include "soc/i2s_struct.h"
#include "soc/spi_struct.h"

__BEGIN_DECLS

/****************************************************************************
 * RC FAST SCLK is optional powered, need to track refcount
 ****************************************************************************/
extern __attribute__((nothrow))
    unsigned CLK_SCLK_RC_FAST_ref(void);
extern __attribute__((nothrow))
    unsigned CLK_SCLK_RC_FAST_release(void);

/****************************************************************************
 * peripherals source clocks selection
 ****************************************************************************/
extern __attribute__((nothrow, const))
    uint64_t CLK_rtc_fast_sclk_freq(void);
extern __attribute__((nothrow))
    int CLK_rtc_fast_sclk_sel(enum RTC_FAST_sclk_sel_t sel, uint32_t div);

    // UART
extern __attribute__((nothrow))
    int CLK_uart_sclk_sel(uart_dev_t *dev, enum UART_sclk_sel_t sel);
extern __attribute__((nothrow))
    uint64_t CLK_uart_sclk_freq(uart_dev_t *dev);

    // I2C
extern __attribute__((nothrow))
    int CLK_i2c_sclk_sel(i2c_dev_t *dev, enum I2C_sclk_sel_t sel);
extern __attribute__((nothrow))
    uint64_t CLK_i2c_sclk_freq(i2c_dev_t *dev);

    // I2S: rx
extern __attribute__((nothrow))
    int CLK_i2s_rx_sclk_sel(i2s_dev_t *dev, enum I2S_sclk_sel_t sel);
extern __attribute__((nothrow))
    uint64_t CLK_i2S_rx_sclk_freq(i2s_dev_t *dev);
    // I2S: tx
extern __attribute__((nothrow))
    int CLK_i2s_tx_sclk_sel(i2s_dev_t *dev, enum I2S_sclk_sel_t sel);
extern __attribute__((nothrow))
    uint64_t CLK_i2S_tx_sclk_freq(i2s_dev_t *dev);

__END_DECLS
#endif
