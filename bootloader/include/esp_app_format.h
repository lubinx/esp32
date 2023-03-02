/*
 * SPDX-FileCopyrightText: 2015-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#pragma once

#include <inttypes.h>
#include "esp_assert.h"

/**
 * @brief ESP chip ID
 *
 */
typedef enum
{
    ESP_CHIP_ID_ESP32 = 0x0000,
    ESP_CHIP_ID_ESP32S2 = 0x0002,
    ESP_CHIP_ID_ESP32C3 = 0x0005,
    ESP_CHIP_ID_ESP32S3 = 0x0009,
    ESP_CHIP_ID_ESP32C2 = 0x000C,
#if CONFIG_IDF_TARGET_ESP32H4_BETA_VERSION_2
    ESP_CHIP_ID_ESP32H4 = 0x000E,
#elif CONFIG_IDF_TARGET_ESP32H4_BETA_VERSION_1
    ESP_CHIP_ID_ESP32H4 = 0x000A,
#endif
    ESP_CHIP_ID_ESP32C6 = 0x000D,
    ESP_CHIP_ID_INVALID = 0xFFFF
} __attribute__((packed)) esp_chip_id_t;

/** @cond */
ESP_STATIC_ASSERT(sizeof(esp_chip_id_t) == 2, "esp_chip_id_t should be 16 bit");
/** @endcond */

/**
 * @brief SPI flash mode, used in esp_image_header_t
 */
typedef enum
{
    ESP_IMAGE_SPI_MODE_QIO,
    ESP_IMAGE_SPI_MODE_QOUT,
    ESP_IMAGE_SPI_MODE_DIO,
    ESP_IMAGE_SPI_MODE_DOUT,
    ESP_IMAGE_SPI_MODE_FAST_READ,
    ESP_IMAGE_SPI_MODE_SLOW_READ
} esp_image_spi_mode_t;

/**
 * @brief SPI flash clock division factor.
 */
typedef enum
{
    ESP_IMAGE_SPI_SPEED_DIV_2,
    ESP_IMAGE_SPI_SPEED_DIV_3,
    ESP_IMAGE_SPI_SPEED_DIV_4,
    ESP_IMAGE_SPI_SPEED_DIV_1 = 0xF
} esp_image_spi_freq_t;

/**
 * @brief Supported SPI flash sizes
 */
typedef enum
{
    ESP_IMAGE_FLASH_SIZE_1MB = 0,
    ESP_IMAGE_FLASH_SIZE_2MB,
    ESP_IMAGE_FLASH_SIZE_4MB,
    ESP_IMAGE_FLASH_SIZE_8MB,
    ESP_IMAGE_FLASH_SIZE_16MB,
    ESP_IMAGE_FLASH_SIZE_32MB,
    ESP_IMAGE_FLASH_SIZE_64MB,
    ESP_IMAGE_FLASH_SIZE_128MB,
    ESP_IMAGE_FLASH_SIZE_MAX
} esp_image_flash_size_t;

#define ESP_IMAGE_HEADER_MAGIC 0xE9

typedef struct
{
    uint8_t magic;              /*!< Magic word ESP_IMAGE_HEADER_MAGIC */
    uint8_t segment_count;      /*!< Count of memory segments */
    uint8_t spi_mode;           /*!< flash read mode (esp_image_spi_mode_t as uint8_t) */
    uint8_t spi_speed: 4;       /*!< flash frequency (esp_image_spi_freq_t as uint8_t) */
    uint8_t spi_size: 4;        /*!< flash chip size (esp_image_flash_size_t as uint8_t) */
    uint32_t entry_addr;        /*!< Entry address */
    uint8_t wp_pin;            /*!< WP pin when SPI pins set via efuse (read by ROM bootloader,
                                * the IDF bootloader uses software to configure the WP
                                * pin and sets this field to 0xEE=disabled) */
    uint8_t spi_pin_drv[3];     /*!< Drive settings for the SPI flash pins (read by ROM bootloader) */
    esp_chip_id_t chip_id;      /*!< Chip identification number */
    uint8_t min_chip_rev;       /*!< Minimal chip revision supported by image
                                 * After the Major and Minor revision eFuses were introduced into the chips, this field is no longer used.
                                 * But for compatibility reasons, we keep this field and the data in it.
                                 * Use min_chip_rev_full instead.
                                 * The software interprets this as a Major version for most of the chips and as a Minor version for the ESP32-C3.
                                 */
    uint16_t min_chip_rev_full; /*!< Minimal chip revision supported by image, in format: major * 100 + minor */
    uint16_t max_chip_rev_full; /*!< Maximal chip revision supported by image, in format: major * 100 + minor */
    uint8_t reserved[4];        /*!< Reserved bytes in additional header space, currently unused */
    uint8_t hash_appended;      /*!< If 1, a SHA256 digest "simple hash" (of the entire image) is appended after the checksum.
                                 * Included in image length. This digest
                                 * is separate to secure boot and only used for detecting corruption.
                                 * For secure boot signed images, the signature
                                 * is appended after this (and the simple hash is included in the signed data). */
} __attribute__((packed))  esp_image_header_t;

/** @cond */
ESP_STATIC_ASSERT(sizeof(esp_image_header_t) == 24, "binary image header should be 24 bytes");
/** @endcond */

typedef struct
{
    uint32_t load_addr;     /*!< Address of segment */
    uint32_t data_len;      /*!< Length of data */
} esp_image_segment_header_t;

#define ESP_IMAGE_MAX_SEGMENTS 16
