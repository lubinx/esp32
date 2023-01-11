/*
 * SPDX-FileCopyrightText: 2020-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#pragma once

#include <features.h>

#include "esp_err.h"
// #include "esp_private/spi_flash_os.h"

__BEGIN_DECLS

/**
 * @brief Read flash ID by sending RDID command (0x9F)
 * @return flash raw ID
 *     mfg_id = (ID >> 16) & 0xFF;
       flash_id = ID & 0xffff;
 */
extern __attribute__((nothrow))
    uint32_t bootloader_read_flash_id(void);

#if SOC_CACHE_SUPPORT_WRAP
    typedef enum
    {
        FLASH_WRAP_MODE_8B = 0,
        FLASH_WRAP_MODE_16B = 2,
        FLASH_WRAP_MODE_32B = 4,
        FLASH_WRAP_MODE_64B = 6,
        FLASH_WRAP_MODE_DISABLE = 1
    } spi_flash_wrap_mode_t;

    extern __attribute__((nothrow))
        esp_err_t bootloader_flash_wrap_set(spi_flash_wrap_mode_t mode);
#endif

extern __attribute__((nothrow))
    esp_err_t bootloader_flash_reset_chip(void);

extern __attribute__((nothrow))
    bool bootloader_flash_is_octal_mode_enabled(void);


    #define FLASH_SECTOR_SIZE           0x1000
    #define FLASH_BLOCK_SIZE 	        0x10000

    #define CMD_RDID       0x9F
    #define CMD_WRSR       0x01
    #define CMD_WRSR2      0x31 /* Not all SPI flash uses this command */
    #define CMD_WRSR3      0x11 /* Not all SPI flash uses this command */
    #define CMD_WREN       0x06
    #define CMD_WRENVSR    0x50 /* Flash write enable for volatile SR bits */
    #define CMD_WRDI       0x04
    #define CMD_RDSR       0x05
    #define CMD_RDSR2      0x35 /* Not all SPI flash uses this command */
    #define CMD_RDSR3      0x15 /* Not all SPI flash uses this command */
    #define CMD_OTPEN      0x3A /* Enable OTP mode, not all SPI flash uses this command */
    #define CMD_RDSFDP     0x5A /* Read the SFDP of the flash */
    #define CMD_WRAP       0x77 /* Set burst with wrap command */
    #define CMD_RESUME     0x7A /* Resume command to clear flash suspend bit */
    #define CMD_RESETEN    0x66
    #define CMD_RESET      0x99

    /**
     * @brief Execute a user command on the flash
     *
     * @param command The command value to execute.
     * @param mosi_data MOSI data to send
     * @param mosi_len Length of MOSI data, in bits
     * @param miso_len Length of MISO data to receive, in bits
     * @return Received MISO data
     */
extern __attribute__((nothrow))
    uint32_t bootloader_execute_flash_command(uint8_t command, uint32_t mosi_data, uint8_t mosi_len, uint8_t miso_len);

__END_DECLS
