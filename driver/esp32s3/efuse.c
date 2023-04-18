#include <stdint.h>
#include <string.h>
#include <sys/errno.h>
// #include <soc/blk_iter.h>
#include <soc/efuse_struct.h>

#include "efuse.h"

/****************************************************************************
 * @def
 ****************************************************************************/
#define BITS_maskof(HIGH, LOW)      \
    (((1UL << ((HIGH) - (LOW) + 1)) - 1) << (LOW))

#define BITS_value(SRC, HIGH, LOW)  \
    (((SRC) & BITS_maskof(HIGH, LOW)) >> LOW)

#define BITS_setvalue(DEST, HIGH, LOW, VALUE)   \
    ((DEST) &= ~BITS_maskof(HIGH, LOW), (DEST) |= ((unsigned)(VALUE)) << (LOW))

enum EFUSE_block
{
    EFUSE_BLK0                      = 0,
    EFUSE_BLK1,
    EFUSE_BLK2,
    EFUSE_BLK3,
    EFUSE_BLK4,
    EFUSE_BLK5,
    EFUSE_BLK6,
    EFUSE_BLK7,
    EFUSE_BLK8,
    EFUSE_BLK9,
    EFUSE_BLK10,
};

struct EFUSE_desc_t
{
    enum EFUSE_block    blk: 8;
    uint8_t             bit_start;
    uint16_t            bit_count;
};

/****************************************************************************
 * @internal
 ****************************************************************************/
uint32_t volatile *const BLK_offset[] =
{
    [EFUSE_BLK0]                                    = &EFUSE.rd_wr_dis,
    [EFUSE_BLK1]                                    = &EFUSE.rd_mac_spi_sys_0,
    [EFUSE_BLK2]                                    = &EFUSE.rd_sys_part1_data0,
    [EFUSE_BLK3]                                    = &EFUSE.rd_usr_data0,
    [EFUSE_BLK4]                                    = &EFUSE.rd_key0_data0,
    [EFUSE_BLK5]                                    = &EFUSE.rd_key1_data0,
    [EFUSE_BLK6]                                    = &EFUSE.rd_key2_data0,
    [EFUSE_BLK7]                                    = &EFUSE.rd_key3_data0,
    [EFUSE_BLK8]                                    = &EFUSE.rd_key4_data0,
    [EFUSE_BLK9]                                    = &EFUSE.rd_key5_data0,
    [EFUSE_BLK10]                                   = &EFUSE.rd_sys_part2_data0,
};

struct EFUSE_desc_t const efuse_desc_table[] =
{
    [EFUSE_FIELD_WR_DIS]                            = {EFUSE_BLK0,   0,  32},
    [EFUSE_FIELD_RD_DIS]                            = {EFUSE_BLK0,  32,   7},
    [EFUSE_FIELD_DIS_ICACHE]                        = {EFUSE_BLK0,  40,   1},
    [EFUSE_FIELD_DIS_DCACHE]                        = {EFUSE_BLK0,  41,   1},
    [EFUSE_FIELD_DIS_DOWNLOAD_ICACHE]               = {EFUSE_BLK0,  42,   1},
    [EFUSE_FIELD_DIS_DOWNLOAD_DCACHE]               = {EFUSE_BLK0,  43,   1},
    [EFUSE_FIELD_DIS_FORCE_DOWNLOAD]                = {EFUSE_BLK0,  44,   1},
    [EFUSE_FIELD_DIS_USB_OTG]                       = {EFUSE_BLK0,  45,   1},
    [EFUSE_FIELD_DIS_TWAI]                          = {EFUSE_BLK0,  46,   1},
    [EFUSE_FIELD_DIS_APP_CPU]                       = {EFUSE_BLK0,  47,   1},
    [EFUSE_FIELD_SOFT_DIS_JTAG]                     = {EFUSE_BLK0,  48,   3},
    [EFUSE_FIELD_DIS_PAD_JTAG]                      = {EFUSE_BLK0,  51,   1},
    [EFUSE_FIELD_DIS_DOWNLOAD_MANUAL_ENCRYPT]       = {EFUSE_BLK0,  52,   1},
    [EFUSE_FIELD_USB_EXCHG_PINS]                    = {EFUSE_BLK0,  57,   1},
    [EFUSE_FIELD_USB_EXT_PHY_ENABLE]                = {EFUSE_BLK0,  58,   1},
    [EFUSE_FIELD_VDD_SPI_XPD]                       = {EFUSE_BLK0,  68,   1},
    [EFUSE_FIELD_VDD_SPI_TIEH]                      = {EFUSE_BLK0,  69,   1},
    [EFUSE_FIELD_VDD_SPI_FORCE]                     = {EFUSE_BLK0,  70,   1},
    [EFUSE_FIELD_WDT_DELAY_SEL]                     = {EFUSE_BLK0,  80,   2},
    [EFUSE_FIELD_SPI_BOOT_CRYPT_CNT]                = {EFUSE_BLK0,  82,   3},
    [EFUSE_FIELD_SECURE_BOOT_KEY_REVOKE0]           = {EFUSE_BLK0,  85,   1},
    [EFUSE_FIELD_SECURE_BOOT_KEY_REVOKE1]           = {EFUSE_BLK0,  86,   1},
    [EFUSE_FIELD_SECURE_BOOT_KEY_REVOKE2]           = {EFUSE_BLK0,  87,   1},
    [EFUSE_FIELD_KEY_PURPOSE_0]                     = {EFUSE_BLK0,  88,   4},
    [EFUSE_FIELD_KEY_PURPOSE_1]                     = {EFUSE_BLK0,  92,   4},
    [EFUSE_FIELD_KEY_PURPOSE_2]                     = {EFUSE_BLK0,  96,   4},
    [EFUSE_FIELD_KEY_PURPOSE_3]                     = {EFUSE_BLK0, 100,   4},
    [EFUSE_FIELD_KEY_PURPOSE_4]                     = {EFUSE_BLK0, 104,   4},
    [EFUSE_FIELD_KEY_PURPOSE_5]                     = {EFUSE_BLK0, 108,   4},
    [EFUSE_FIELD_SECURE_BOOT_EN]                    = {EFUSE_BLK0, 116,   1},
    [EFUSE_FIELD_SECURE_BOOT_AGGRESSIVE_REVOKE]     = {EFUSE_BLK0, 117,   1},
    [EFUSE_FIELD_DIS_USB_JTAG]                      = {EFUSE_BLK0, 118,   1},
    [EFUSE_FIELD_DIS_USB_SERIAL_JTAG]               = {EFUSE_BLK0, 119,   1},
    [EFUSE_FIELD_STRAP_JTAG_SEL]                    = {EFUSE_BLK0, 120,   1},
    [EFUSE_FIELD_USB_PHY_SEL]                       = {EFUSE_BLK0, 121,   1},
    [EFUSE_FIELD_FLASH_TPUW]                        = {EFUSE_BLK0, 124,   4},
    [EFUSE_FIELD_DIS_DOWNLOAD_MODE]                 = {EFUSE_BLK0, 128,   1},
    [EFUSE_FIELD_DIS_DIRECT_BOOT]                   = {EFUSE_BLK0, 129,   1},
    [EFUSE_FIELD_DIS_USB_SERIAL_JTAG_ROM_PRINT]     = {EFUSE_BLK0, 130,   1},
    [EFUSE_FIELD_FLASH_ECC_MODE]                    = {EFUSE_BLK0, 131,   1},
    [EFUSE_FIELD_DIS_USB_SERIAL_JTAG_DOWNLOAD_MODE] = {EFUSE_BLK0, 132,   1},
    [EFUSE_FIELD_ENABLE_SECURITY_DOWNLOAD]          = {EFUSE_BLK0, 133,   1},
    [EFUSE_FIELD_UART_PRINT_CONTROL]                = {EFUSE_BLK0, 134,   2},
    [EFUSE_FIELD_PIN_POWER_SELECTION]               = {EFUSE_BLK0, 136,   1},
    [EFUSE_FIELD_FLASH_TYPE]                        = {EFUSE_BLK0, 137,   1},
    [EFUSE_FIELD_FLASH_PAGE_SIZE]                   = {EFUSE_BLK0, 138,   2},
    [EFUSE_FIELD_FLASH_ECC_EN]                      = {EFUSE_BLK0, 140,   1},
    [EFUSE_FIELD_FORCE_SEND_RESUME]                 = {EFUSE_BLK0, 141,   1},
    [EFUSE_FIELD_SECURE_VERSION]                    = {EFUSE_BLK0, 142,  16},
    [EFUSE_FIELD_DIS_USB_OTG_DOWNLOAD_MODE]         = {EFUSE_BLK0, 159,   1},
    [EFUSE_FIELD_DISABLE_WAFER_VERSION_MAJOR]       = {EFUSE_BLK0, 160,   1},
    [EFUSE_FIELD_DISABLE_BLK_VERSION_MAJOR]         = {EFUSE_BLK0, 161,   1},
    [EFUSE_FIELD_MAC]                               = {EFUSE_BLK1,   0,  48},
    [EFUSE_FIELD_SPI_PAD_CONFIG_CLK]                = {EFUSE_BLK1,  48,   6},
    [EFUSE_FIELD_SPI_PAD_CONFIG_Q]                  = {EFUSE_BLK1,  54,   6},
    [EFUSE_FIELD_SPI_PAD_CONFIG_D]                  = {EFUSE_BLK1,  60,   6},
    [EFUSE_FIELD_SPI_PAD_CONFIG_CS]                 = {EFUSE_BLK1,  66,   6},
    [EFUSE_FIELD_SPI_PAD_CONFIG_HD]                 = {EFUSE_BLK1,  72,   6},
    [EFUSE_FIELD_SPI_PAD_CONFIG_WP]                 = {EFUSE_BLK1,  78,   6},
    [EFUSE_FIELD_SPI_PAD_CONFIG_DQS]                = {EFUSE_BLK1,  84,   6},
    [EFUSE_FIELD_SPI_PAD_CONFIG_D4]                 = {EFUSE_BLK1,  90,   6},
    [EFUSE_FIELD_SPI_PAD_CONFIG_D5]                 = {EFUSE_BLK1,  96,   6},
    [EFUSE_FIELD_SPI_PAD_CONFIG_D6]                 = {EFUSE_BLK1, 102,   6},
    [EFUSE_FIELD_SPI_PAD_CONFIG_D7]                 = {EFUSE_BLK1, 108,   6},
    [EFUSE_FIELD_WAFER_VERSION_MINOR_LO]            = {EFUSE_BLK1, 114,   3},
    [EFUSE_FIELD_PKG_VERSION]                       = {EFUSE_BLK1, 117,   3},
    [EFUSE_FIELD_BLK_VERSION_MINOR]                 = {EFUSE_BLK1, 120,   3},
    [EFUSE_FIELD_K_RTC_LDO]                         = {EFUSE_BLK1, 141,   7},
    [EFUSE_FIELD_K_DIG_LDO]                         = {EFUSE_BLK1, 148,   7},
    [EFUSE_FIELD_V_RTC_DBIAS20]                     = {EFUSE_BLK1, 155,   8},
    [EFUSE_FIELD_V_DIG_DBIAS20]                     = {EFUSE_BLK1, 163,   8},
    [EFUSE_FIELD_DIG_DBIAS_HVT]                     = {EFUSE_BLK1, 171,   5},
    [EFUSE_FIELD_WAFER_VERSION_MINOR_HI]            = {EFUSE_BLK1, 183,   1},
    [EFUSE_FIELD_WAFER_VERSION_MAJOR]               = {EFUSE_BLK1, 184,   2},
    [EFUSE_FIELD_ADC2_CAL_VOL_ATTEN3]               = {EFUSE_BLK1, 186,   6},
    [EFUSE_FIELD_OPTIONAL_UNIQUE_ID]                = {EFUSE_BLK2,   0, 128},
    [EFUSE_FIELD_BLK_VERSION_MAJOR]                 = {EFUSE_BLK2, 128,   2},
    [EFUSE_FIELD_TEMP_CALIB]                        = {EFUSE_BLK2, 132,   9},
    [EFUSE_FIELD_OCODE]                             = {EFUSE_BLK2, 141,   8},
    [EFUSE_FIELD_ADC1_INIT_CODE_ATTEN0]             = {EFUSE_BLK2, 149,   8},
    [EFUSE_FIELD_ADC1_INIT_CODE_ATTEN1]             = {EFUSE_BLK2, 157,   6},
    [EFUSE_FIELD_ADC1_INIT_CODE_ATTEN2]             = {EFUSE_BLK2, 163,   6},
    [EFUSE_FIELD_ADC1_INIT_CODE_ATTEN3]             = {EFUSE_BLK2, 169,   6},
    [EFUSE_FIELD_ADC2_INIT_CODE_ATTEN0]             = {EFUSE_BLK2, 175,   8},
    [EFUSE_FIELD_ADC2_INIT_CODE_ATTEN1]             = {EFUSE_BLK2, 183,   6},
    [EFUSE_FIELD_ADC2_INIT_CODE_ATTEN2]             = {EFUSE_BLK2, 189,   6},
    [EFUSE_FIELD_ADC2_INIT_CODE_ATTEN3]             = {EFUSE_BLK2, 195,   6},
    [EFUSE_FIELD_ADC1_CAL_VOL_ATTEN0]               = {EFUSE_BLK2, 201,   8},
    [EFUSE_FIELD_ADC1_CAL_VOL_ATTEN1]               = {EFUSE_BLK2, 209,   8},
    [EFUSE_FIELD_ADC1_CAL_VOL_ATTEN2]               = {EFUSE_BLK2, 217,   8},
    [EFUSE_FIELD_ADC1_CAL_VOL_ATTEN3]               = {EFUSE_BLK2, 225,   8},
    [EFUSE_FIELD_ADC2_CAL_VOL_ATTEN0]               = {EFUSE_BLK2, 233,   8},
    [EFUSE_FIELD_ADC2_CAL_VOL_ATTEN1]               = {EFUSE_BLK2, 241,   7},
    [EFUSE_FIELD_ADC2_CAL_VOL_ATTEN2]               = {EFUSE_BLK2, 248,   7},
    [EFUSE_FIELD_USER_DATA]                         = {EFUSE_BLK3,   0, 256},
    [EFUSE_FIELD_KEY0]                              = {EFUSE_BLK4,   0, 256},
    [EFUSE_FIELD_KEY1]                              = {EFUSE_BLK5,   0, 256},
    [EFUSE_FIELD_KEY2]                              = {EFUSE_BLK6,   0, 256},
    [EFUSE_FIELD_KEY3]                              = {EFUSE_BLK7,   0, 256},
    [EFUSE_FIELD_KEY4]                              = {EFUSE_BLK8,   0, 256},
    [EFUSE_FIELD_KEY5]                              = {EFUSE_BLK9,   0, 256},
    [EFUSE_FIELD_SYS_DATA_PART2]                    = {EFUSE_BLK10,  0, 256},
};

/****************************************************************************
 * @implements
 ****************************************************************************/
int EFUSE_read_blob(enum EFUSE_field field, void *dst, size_t bits_size)
{
    if (lengthof(efuse_desc_table) - 1 < (unsigned)field)
        return EINVAL;

    struct EFUSE_desc_t const *desc = &efuse_desc_table[field];
    if ((7 & desc->bit_start) || (7 & desc->bit_count)) // impossiable alignment, blob field must alignto byte
        return EINVAL;
    if (bits_size != desc->bit_count)
        return EINVAL;

    uint32_t volatile *blk_iter = BLK_offset[desc->blk + desc->bit_start / (sizeof(uint32_t) * 8)];

    if ((sizeof(uint32_t) * 8 - 1) & desc->bit_start)
    {
        uint32_t regval = *blk_iter;

        while (0 < bits_size)
        {
            *(uint8_t *)dst ++ = (uint8_t)(regval >> 24);

            regval <<= 8;
            bits_size -= 8;
        }
        blk_iter ++;
    }

    while (31 < bits_size)
    {
        uint32_t regval = *blk_iter;
        memcpy(dst, &regval, sizeof(regval));

        dst = (uint8_t *)dst + sizeof(uint32_t);
        blk_iter ++;
        bits_size -= 32;
    }

    if (0 < bits_size)
    {
        uint32_t regval = *blk_iter;

        while (0 < bits_size)
        {
            *(uint8_t *)dst ++ = 0XFFU & regval;

            regval >>= 8;
            bits_size -= 8;
        }
    }
    return 0;
}

int EFUSE_read_value(enum EFUSE_field field, unsigned *value)
{
    if (lengthof(efuse_desc_table) - 1 < (unsigned)field)
        return EINVAL;

    *value = 0;
    return 0;
}
