#ifndef __ESP32S3_I2C_H
#define __ESP32S3_I2C_H                 1

#include <features.h>

#include "hw/i2c.h"
#include "soc/i2c_struct.h"

__BEGIN_DECLS

extern __attribute__((nothrow, nonnull))
    int I2C_configure(uart_dev_t *dev, uint32_t kbps);
extern __attribute__((nothrow, nonnull))
    int I2C_deconfigure(uart_dev_t *dev);

__END_DECLS
