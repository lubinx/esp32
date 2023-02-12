#pragma once

#include <features.h>

#include "hw/uart.h"
#include "soc/soc_caps.h"
#include "soc/uart_struct.h"

    // ref 26.3
    /* REVIEW: esp-idf driver UART_SCLK_RTC = RTC_CLK_FREQ which is 20M
        but...find nowwhere of this clock
        in reference document:
            1：APB_CLK 2：FOSC_CLK 3：XTAL_CLK, FOSC is INT_RC_FAST? which is 17.5M
    */
    enum uart_sclk_sel_t
    {
        UART_SCLK_SEL_APB           = 1,
        UART_SCLK_SEL_INT_RC_FAST,
        UART_SCLK_SEL_XTAL
    };
    typedef enum uart_sclk_sel_t    uart_sclk_sel_t;

__BEGIN_DECLS

extern __attribute__((nothrow, nonnull))
    int UART_configure(uart_dev_t *dev, uart_sclk_sel_t sclk, uint32_t bps, parity_t parity, uint8_t stop_bits);
extern __attribute__((nothrow, nonnull))
    int UART_deconfigure(uart_dev_t *dev);

extern __attribute__((nothrow, nonnull, const))
    uint32_t UART_get_baudrate(uart_dev_t *dev);

    /**
     *  fifo tx char
     *      @note gurantee successful
    */
static inline
    void UART_fifo_tx(uart_dev_t *dev, uint8_t ch)
    {
        while (SOC_UART_FIFO_LEN == dev->status.txfifo_cnt);
        dev->fifo.rxfifo_rd_byte = ch;
    }

    /**
     *  fifo rx char
     *      @note gurantee successful
    */
static inline
    uint8_t UART_fifo_rx(uart_dev_t *dev)
    {
        while (0 == dev->status.rxfifo_cnt);
        return (uint8_t)dev->fifo.rxfifo_rd_byte;
    }

    /**
     *  write fifo buffer
     *      @returns counts written until fifo is full
     *      @note possiable return 0
    */
static inline
    int UART_fifo_write(uart_dev_t *dev, void const *buf, unsigned count)
    {
        unsigned written = 0;
        while (written < count)
        {
             if (SOC_UART_FIFO_LEN == dev->status.txfifo_cnt)
                break;
             dev->fifo.rxfifo_rd_byte = *((uint8_t *)buf + written);
             written ++;
        }
        return written;
    }

    /**
     *  read fifo buffer
     *      @returns counts readed until fifo is empty
     *      @note possiable return 0
    */
static inline
    int UART_fifo_read(uart_dev_t *dev, void *buf, unsigned bufsize)
    {
        unsigned readed = 0;
        while (readed < bufsize)
        {
            if (0 == dev->status.rxfifo_cnt)
                break;
            *((uint8_t *)buf + readed) = (uint8_t)dev->fifo.rxfifo_rd_byte;
            readed ++;
        }
        return readed;
    }

__END_DECLS
