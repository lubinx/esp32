#pragma once

#include <features.h>
#include <stdint.h>

struct esp_image_hdr_t
{
    uint8_t magic;
    uint8_t segment_count;
    uint8_t spi_mode;               // enum esp_image_spi_mode_t
    uint8_t spi_speed: 4;           // enum esp_image_spi_freq_t
    uint8_t spi_size: 4;            // enum esp_image_flash_size_t

    uint32_t entry_addr;

    // WP pin when SPI pins set via efuse (read by ROM bootloader,
        // * the IDF bootloader uses software to configure the WP
        // * pin and sets this field to 0xEE=disabled) */
    uint8_t wp_pin;
     // Drive settings for the SPI flash pins (read by ROM bootloader)
    uint8_t spi_pin_drv[3];

    uint16_t chip_id;               // enum esp_chip_id

    // deprecated
    uint8_t reserved[9];
    /*
    uint8_t min_chip_rev;
    uint16_t min_chip_rev_full;
    uint16_t max_chip_rev_full;
    uint8_t reserved[4];
    */
    uint8_t hash_appended;
};
typedef struct esp_image_hdr_t   esp_image_hdr_t;

#define ESP_IMAGE_HEADER_MAGIC          (0xE9)
#define ESP_IMAGE_MAX_SEGMENTS          (16)

static_assert(24 == sizeof(esp_image_hdr_t), "binary image header should be 24 bytes");

struct esp_segment_hdr_t
{
    uint32_t load_addr;     /*!< Address of segment */
    uint32_t data_len;      /*!< Length of data */
};
typedef struct esp_segment_hdr_t    esp_segment_hdr_t;

enum esp_chip_id
{
    ESP_CHIP_ID_ESP32 = 0x0000,
    ESP_CHIP_ID_ESP32S2 = 0x0002,
    ESP_CHIP_ID_ESP32C3 = 0x0005,
    ESP_CHIP_ID_ESP32S3 = 0x0009,
    ESP_CHIP_ID_ESP32C2 = 0x000C,
    /* TODO: CHIP ID: removed beta
#if CONFIG_IDF_TARGET_ESP32H4_BETA_VERSION_2
    ESP_CHIP_ID_ESP32H4 = 0x000E,
#elif CONFIG_IDF_TARGET_ESP32H4_BETA_VERSION_1
    ESP_CHIP_ID_ESP32H4 = 0x000A,
#endif
    */
    ESP_CHIP_ID_ESP32C6 = 0x000D,
    ESP_CHIP_ID_INVALID = 0xFFFF
};

enum esp_image_spi_mode_t
{
    ESP_IMAGE_SPI_MODE_QIO,
    ESP_IMAGE_SPI_MODE_QOUT,
    ESP_IMAGE_SPI_MODE_DIO,
    ESP_IMAGE_SPI_MODE_DOUT,
    ESP_IMAGE_SPI_MODE_FAST_READ,
    ESP_IMAGE_SPI_MODE_SLOW_READ
};

enum esp_image_spi_freq_t
{
    ESP_IMAGE_SPI_SPEED_DIV_2,
    ESP_IMAGE_SPI_SPEED_DIV_3,
    ESP_IMAGE_SPI_SPEED_DIV_4,
    ESP_IMAGE_SPI_SPEED_DIV_1 = 0xF
};

enum esp_image_flash_size_t
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
};
