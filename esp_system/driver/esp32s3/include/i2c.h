#ifndef __ESP32S3_I2C_H
#define __ESP32S3_I2C_H                 1

#include <features.h>

#include "hw/i2c.h"
#include "soc/i2c_struct.h"

__BEGIN_DECLS

/****************************************************************************
 *  configure
 ****************************************************************************/
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
    uint32_t I2C_get_bps(i2c_dev_t *dev);

/****************************************************************************
 *  I2C dev IO
 ****************************************************************************/
extern __attribute__((nothrow, nonnull(1, 5)))
    ssize_t I2C_dev_pread(i2c_dev_t *dev, uint16_t da, uint8_t sa_bytes, uint32_t sa, void *buf, size_t bufsize);

static inline __attribute__((nothrow, nonnull))
    int I2C_dev_read(i2c_dev_t *dev, uint16_t da, void *buf, size_t bufsize)
    {
        return I2C_dev_pread(dev, da, 0, 0, buf, bufsize);
    }

extern __attribute__((nothrow, nonnull(1, 5)))
    ssize_t I2C_dev_pwrite(i2c_dev_t *dev, uint16_t da, uint8_t sa_bytes, uint32_t sa, void const *buf, size_t count);

static inline __attribute__((nothrow, nonnull))
    int I2C_dev_write(i2c_dev_t *dev, uint16_t da, void *buf, size_t bufsize)
    {
        return I2C_dev_pwrite(dev, da, 0, 0, buf, bufsize);
    }

__END_DECLS
#endif
