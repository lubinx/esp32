/*
 * SPDX-FileCopyrightText: 2015-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __ESP_LOG_H__
#define __ESP_LOG_H__

#include <stdint.h>
#include <stdarg.h>
#include <inttypes.h>

#pragma GCC diagnostic ignored "-Wformat"

#ifdef __cplusplus
extern "C" {
#endif

    enum esp_log_level_t
    {
        ESP_LOG_NONE,       /*!< No log output */
        ESP_LOG_ERROR,      /*!< Critical errors, software module can not recover on its own */
        ESP_LOG_WARN,       /*!< Error conditions from which recovery measures have been taken */
        ESP_LOG_INFO,       /*!< Information messages which describe normal flow of events */
        ESP_LOG_DEBUG,      /*!< Extra information which is not necessary for normal use (values, pointers, sizes, etc). */
        ESP_LOG_VERBOSE     /*!< Bigger chunks of debugging information, or frequent messages which can potentially flood the output. */
    };
    typedef enum esp_log_level_t esp_log_level_t;

    #define ESP_LOG_LEVEL(level, tag, format, ...)  \
        esp_log_write(level, tag, format)
    #define ESP_LOG_LEVEL_LOCAL(level, tag, format, ...)    \
        esp_log_write(level, tag, format)

    #define ESP_EARLY_LOGE(tag, format, ...)    esp_log_write(ESP_LOG_VERBOSE, tag, format, ##__VA_ARGS__)
    #define ESP_EARLY_LOGW(tag, format, ...)    esp_log_write(ESP_LOG_VERBOSE, tag, format, ##__VA_ARGS__)
    #define ESP_EARLY_LOGI(tag, format, ...)    esp_log_write(ESP_LOG_VERBOSE, tag, format, ##__VA_ARGS__)
    #define ESP_EARLY_LOGD(tag, format, ...)    esp_log_write(ESP_LOG_VERBOSE, tag, format, ##__VA_ARGS__)
    #define ESP_EARLY_LOGV(tag, format, ...)    esp_log_write(ESP_LOG_VERBOSE, tag, format, ##__VA_ARGS__)

    #define ESP_LOGE(tag, format, ...)          esp_log_write(ESP_LOG_VERBOSE, tag, format, ##__VA_ARGS__)
    #define ESP_LOGW(tag, format, ...)          esp_log_write(ESP_LOG_VERBOSE, tag, format, ##__VA_ARGS__)
    #define ESP_LOGI(tag, format, ...)          esp_log_write(ESP_LOG_VERBOSE, tag, format, ##__VA_ARGS__)
    #define ESP_LOGD(tag, format, ...)          esp_log_write(ESP_LOG_VERBOSE, tag, format, ##__VA_ARGS__)
    #define ESP_LOGV(tag, format, ...)          esp_log_write(ESP_LOG_VERBOSE, tag, format, ##__VA_ARGS__)

    #define ESP_DRAM_LOGE(tag, format, ...)     esp_log_write(ESP_LOG_VERBOSE, tag, format, ##__VA_ARGS__)
    #define ESP_DRAM_LOGW(tag, format, ...)     esp_log_write(ESP_LOG_VERBOSE, tag, format, ##__VA_ARGS__)
    #define ESP_DRAM_LOGI(tag, format, ...)     esp_log_write(ESP_LOG_VERBOSE, tag, format, ##__VA_ARGS__)
    #define ESP_DRAM_LOGD(tag, format, ...)     esp_log_write(ESP_LOG_VERBOSE, tag, format, ##__VA_ARGS__)
    #define ESP_DRAM_LOGV(tag, format, ...)     esp_log_write(ESP_LOG_VERBOSE, tag, format, ##__VA_ARGS__)

    #define ESP_LOG_BUFFER_HEX_LEVEL(tag, buffer, buff_len, level)
    #define ESP_LOG_BUFFER_CHAR_LEVEL(tag, buffer, buff_len, level)
    #define ESP_LOG_BUFFER_HEXDUMP(tag, buffer, buff_len, level)
    #define ESP_LOG_BUFFER_HEX(tag, buffer, buff_len)
    #define ESP_LOG_BUFFER_CHAR(tag, buffer, buff_len)

    #define LOG_FORMAT(letter, format)  LOG_COLOR_ ## letter #letter " (%" PRIu32 ") %s: " format LOG_RESET_COLOR "\n"
    #define LOG_SYSTEM_TIME_FORMAT(letter, format)  LOG_COLOR_ ## letter #letter " (%s) %s: " format LOG_RESET_COLOR "\n"

// to be back compatible
    #define LOG_LOCAL_LEVEL  ESP_LOG_NONE
    #define esp_log_buffer_hex                  ESP_LOG_BUFFER_HEX
    #define esp_log_buffer_char                 ESP_LOG_BUFFER_CHAR
/*
    #define _ESP_LOG_DRAM_LOG_FORMAT(letter, format)
    #define ESP_DRAM_LOG_IMPL(tag, format, log_level, log_tag_letter, ...)
*/

#if CONFIG_LOG_COLORS
    #define LOG_COLOR_BLACK   "30"
    #define LOG_COLOR_RED     "31"
    #define LOG_COLOR_GREEN   "32"
    #define LOG_COLOR_BROWN   "33"
    #define LOG_COLOR_BLUE    "34"
    #define LOG_COLOR_PURPLE  "35"
    #define LOG_COLOR_CYAN    "36"
    #define LOG_COLOR(COLOR)  "\033[0;" COLOR "m"
    #define LOG_BOLD(COLOR)   "\033[1;" COLOR "m"
    #define LOG_RESET_COLOR   "\033[0m"
    #define LOG_COLOR_E       LOG_COLOR(LOG_COLOR_RED)
    #define LOG_COLOR_W       LOG_COLOR(LOG_COLOR_BROWN)
    #define LOG_COLOR_I       LOG_COLOR(LOG_COLOR_GREEN)
    #define LOG_COLOR_D
    #define LOG_COLOR_V
#else
    #define LOG_COLOR_E
    #define LOG_COLOR_W
    #define LOG_COLOR_I
    #define LOG_COLOR_D
    #define LOG_COLOR_V
    #define LOG_RESET_COLOR
#endif


    typedef int (*vprintf_like_t)(const char *, va_list);
static inline
    vprintf_like_t esp_log_set_vprintf(vprintf_like_t func)
    {
        return func;
    }

static inline
    void esp_log_level_set(char const *tag, esp_log_level_t level)
    {
        (void)tag;
        (void)level;
    }

static inline
    esp_log_level_t esp_log_level_get(char const *tag)
    {
        (void)tag;
        return ESP_LOG_NONE;
    }

static inline
    uint32_t esp_log_timestamp(void)
    {
        return 0;
    }

static inline
    char const *esp_log_system_timestamp(void)
    {
        return "";
    }

static inline
    uint32_t esp_log_early_timestamp(void)
    {
        return 0;
    }

extern __attribute__((nothrow))
    void esp_log_writev(esp_log_level_t level, char const *tag, char const *format, va_list args);

extern  __attribute__ ((nothrow, format (printf, 3, 4)))
    void esp_log_write(esp_log_level_t level, char const *tag, char const *format, ...);

    // avoid to #include "esp_rom_sys.h"
extern
    void esp_rom_delay_us(uint32_t us);

#ifdef __cplusplus
}
#endif

#endif /* __ESP_LOG_H__ */
