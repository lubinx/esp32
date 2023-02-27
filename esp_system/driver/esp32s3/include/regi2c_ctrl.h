#pragma once

#include <features.h>

#include <stdint.h>

#include "soc/soc.h"
#include "soc/regi2c_defs.h"

__BEGIN_DECLS

// avoid #include "esp_rom_regi2c.h"
extern __attribute__((nothrow))
    uint8_t esp_rom_regi2c_read(uint8_t block, uint8_t host_id, uint8_t reg);
extern __attribute__((nothrow))
    uint8_t esp_rom_regi2c_read_mask(uint8_t block, uint8_t host_id, uint8_t reg, uint8_t msb, uint8_t lsb);
extern __attribute__((nothrow))
    void esp_rom_regi2c_write(uint8_t block, uint8_t host_id, uint8_t reg, uint8_t data);
extern __attribute__((nothrow))
    void esp_rom_regi2c_write_mask(uint8_t block, uint8_t host_id, uint8_t reg, uint8_t msb, uint8_t lsb, uint8_t data);

/* Convenience macros for the above functions, these use register definitions
 * from regi2c_xxx.h header files.
 */
#define REGI2C_WRITE(block, reg, indata) \
    esp_rom_regi2c_write(block, block##_HOSTID, reg, indata)

#define REGI2C_WRITE_MASK(block, reg, indata) \
    esp_rom_regi2c_write_mask(block, block##_HOSTID, reg, reg##_MSB, reg##_LSB, indata)

#define REGI2C_READ(block, reg) \
    esp_rom_regi2c_read(block, block##_HOSTID, reg)

#define REGI2C_READ_MASK(block, reg) \
    esp_rom_regi2c_read_mask(block, block##_HOSTID, reg, reg##_MSB, reg##_LSB)

static inline
    void REGI2C_reset(void)
    {
        SET_PERI_REG_BITS(ANA_CONFIG_REG, ANA_CONFIG_M, ANA_CONFIG_M, ANA_CONFIG_S);
    }

static inline
    void REGI2C_saradc_enable(void)
    {
        CLEAR_PERI_REG_MASK(ANA_CONFIG_REG, I2C_SAR_M);
        SET_PERI_REG_MASK(ANA_CONFIG2_REG, ANA_SAR_CFG2_M);
    }

static inline
    void REGI2C_saradc_disable(void)
    {
        CLEAR_PERI_REG_MASK(ANA_CONFIG2_REG, ANA_SAR_CFG2_M);
    }

static inline
    void REGI2C_bbpll_enable(void)
    {
        CLEAR_PERI_REG_MASK(ANA_CONFIG_REG, I2C_BBPLL_M);
    }

#include "soc/regi2c_bbpll.h"

static inline
    void REGI2C_bbpll_calibration_start(void)
    {
        REG_CLR_BIT(I2C_MST_ANA_CONF0_REG, I2C_MST_BBPLL_STOP_FORCE_HIGH);
        REG_SET_BIT(I2C_MST_ANA_CONF0_REG, I2C_MST_BBPLL_STOP_FORCE_LOW);
    }

static inline
    void REGI2C_bbpll_calibration_end(void)
    {
        while (! REG_GET_BIT(I2C_MST_ANA_CONF0_REG, I2C_MST_BBPLL_CAL_DONE));

        REG_CLR_BIT(I2C_MST_ANA_CONF0_REG, I2C_MST_BBPLL_STOP_FORCE_LOW);
        REG_SET_BIT(I2C_MST_ANA_CONF0_REG, I2C_MST_BBPLL_STOP_FORCE_HIGH);
    }

__END_DECLS
