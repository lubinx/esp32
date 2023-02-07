#pragma once

#include "hw/uart.h"
#include "soc/uart_struct.h"

// ref 26.3
/* REVIEW: esp-idf driver UART_SCLK_RTC = RTC_CLK_FREQ which is 20M
    but...find nowwhere source this clock
    in reference document:
        1：APB_CLK；2：FOSC_CLK；3：XTAL_CLK
        but FOSC is RC_FAST? which is 17.5M
*/
enum soc_uart_sclk_sel_t
{
    SOC_UART_CLK_SRC_APB        = 1,
    SOC_UART_CLK_SRC_RC_FAST,
    SOC_UART_CLK_SRC_XTAL
};
typedef enum soc_uart_sclk_sel_t    soc_uart_sclk_sel_t;

extern __attribute__((nothrow, nonnull))
    int UART_sclk_sel(uart_dev_t *dev, enum soc_uart_sclk_sel_t sel);
