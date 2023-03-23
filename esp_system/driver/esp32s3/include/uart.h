#ifndef __ESP32S3_UART_H
#define __ESP32S3_UART_H                1

#include <features.h>

#include "gpio.h"
#include "hw/uart.h"
#include "soc/uart_struct.h"

__BEGIN_DECLS

/****************************************************************************
 *  configure
 ****************************************************************************/
extern __attribute__((nothrow, nonnull))
    int UART_configure(uart_dev_t *dev, uint32_t bps, enum UART_parity_t parity, enum UART_stopbits_t stopbits);
extern __attribute__((nothrow, nonnull))
    int UART_deconfigure(uart_dev_t *dev);

extern __attribute__((nothrow, nonnull, const))
    uint32_t UART_get_baudrate(uart_dev_t *dev);

/****************************************************************************
 *  direct write UART through fifo
 ****************************************************************************/
    /**
     *  fifo tx a char
     *      @note spin to gurantee successful
    */
extern __attribute__((nothrow, nonnull))
    void UART_fifo_tx(uart_dev_t *dev, uint8_t ch);

    /**
     *  fifo rx a char
     *      @note spin to gurantee successful
    */
extern __attribute__((nothrow, nonnull))
    uint8_t UART_fifo_rx(uart_dev_t *dev);

    /**
     *  write fifo buffer
     *      @returns counts written until fifo is full
     *      @note possiable return 0
    */
extern __attribute__((nothrow, nonnull))
    int UART_fifo_write(uart_dev_t *dev, void const *buf, unsigned count);

    /**
     *  read fifo buffer
     *      @returns counts readed until fifo is empty
     *      @note possiable return 0
    */
extern __attribute__((nothrow, nonnull))
    int UART_fifo_read(uart_dev_t *dev, void *buf, unsigned bufsize);

__END_DECLS
#endif
