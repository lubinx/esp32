#ifndef __ESP32S3_I2C_H
#define __ESP32S3_I2C_H                 1

#include <features.h>

#include "hw/i2c.h"
#include "soc/i2c_struct.h"

__BEGIN_DECLS

    enum I2C_mode_t
    {
        I2C_MASTER_MODE,
        I2C_SLAVE_MODE
    };

extern __attribute__((nothrow, nonnull))
    int I2C_configure(i2c_dev_t *dev, enum I2C_mode_t mode, uint32_t kbps);
extern __attribute__((nothrow, nonnull))
    int I2C_deconfigure(i2c_dev_t *dev);

extern __attribute__((nothrow, const))
    unsigned I2C_get_bps(i2c_dev_t dev);

__END_DECLS
#endif
