/*
 * SPDX-FileCopyrightText: 2015-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __ESP_LOG_H__
#define __ESP_LOG_H__

#include <features.h>

#include <inttypes.h>
#include <stdarg.h>

    enum esp_log_level_t
    {
        ESP_LOG_NONE,
        ESP_LOG_ERROR,
        ESP_LOG_WARN,
        ESP_LOG_INFO,
        ESP_LOG_DEBUG,
        ESP_LOG_VERBOSE
    };
    extern enum esp_log_level_t __log_level;

    #define ESP_LOGE(tag, format, ...)  \
        ESP_LOG_LEVEL(ESP_LOG_ERROR,    tag, format, ##__VA_ARGS__)
    #define ESP_LOGW(tag, format, ...)  \
        ESP_LOG_LEVEL(ESP_LOG_WARN,     tag, format, ##__VA_ARGS__)
    #define ESP_LOGI(tag, format, ...)  \
        ESP_LOG_LEVEL(ESP_LOG_INFO,     tag, format, ##__VA_ARGS__)
    #define ESP_LOGD(tag, format, ...)  \
        ESP_LOG_LEVEL(ESP_LOG_DEBUG,    tag, format, ##__VA_ARGS__)
    #define ESP_LOGV(tag, format, ...)  \
        ESP_LOG_LEVEL(ESP_LOG_VERBOSE,  tag, format, ##__VA_ARGS__)

    #define ESP_EARLY_LOGE(tag, format, ...)    \
        ESP_LOG_LEVEL(ESP_LOG_ERROR,    tag, format, ##__VA_ARGS__)
    #define ESP_EARLY_LOGW(tag, format, ...)    \
        ESP_LOG_LEVEL(ESP_LOG_WARN,     tag, format, ##__VA_ARGS__)
    #define ESP_EARLY_LOGI(tag, format, ...)    \
        ESP_LOG_LEVEL(ESP_LOG_INFO,     tag, format, ##__VA_ARGS__)
    #define ESP_EARLY_LOGD(tag, format, ...)    \
        ESP_LOG_LEVEL(ESP_LOG_DEBUG,    tag, format, ##__VA_ARGS__)
    #define ESP_EARLY_LOGV(tag, format, ...)    \
        ESP_LOG_LEVEL(ESP_LOG_VERBOSE,  tag, format, ##__VA_ARGS__)

    #define ESP_DRAM_LOGE(tag, format, ...)     \
        ESP_LOG_LEVEL(ESP_LOG_ERROR,    tag, format, ##__VA_ARGS__)
    #define ESP_DRAM_LOGW(tag, format, ...)     \
        ESP_LOG_LEVEL(ESP_LOG_WARN,     tag, format, ##__VA_ARGS__)
    #define ESP_DRAM_LOGI(tag, format, ...)     \
        ESP_LOG_LEVEL(ESP_LOG_INFO,     tag, format, ##__VA_ARGS__)
    #define ESP_DRAM_LOGD(tag, format, ...)     \
        ESP_LOG_LEVEL(ESP_LOG_DEBUG,    tag, format, ##__VA_ARGS__)
    #define ESP_DRAM_LOGV(tag, format, ...)     \
        ESP_LOG_LEVEL(ESP_LOG_VERBOSE,  tag, format, ##__VA_ARGS__)

    #define ESP_LOG_LEVEL(level, tag, format, ...)  \
        (level <= __log_level ? esp_log_printf(LOG_FORMAT(level, format), esp_log_timestamp(), tag, ##__VA_ARGS__) : (void)0)
    #define ESP_EARLY_LOG               ESP_LOG_LEVEL

    #define LOG_FORMAT(level, format)   \
        level##_COLOR "%-6" PRIu32 level##_LETTER" %s: " format LOG_END "\n"

    #define LOG_COLOR_BLACK             "30"
    #define LOG_COLOR_RED               "31"
    #define LOG_COLOR_GREEN             "32"
    #define LOG_COLOR_BROWN             "33"
    #define LOG_COLOR_BLUE              "34"
    #define LOG_COLOR_PURPLE            "35"
    #define LOG_COLOR_CYAN              "36"
    #define LOG_COLOR(COLOR)            "\033[0;" COLOR "m"
    #define LOG_BOLD(COLOR)             "\033[1;" COLOR "m"
    #define LOG_RESET_COLOR             "\033[0m"

    #define LOG_END                     LOG_RESET_COLOR
    #define ESP_LOG_ERROR_COLOR         LOG_COLOR(LOG_COLOR_RED)
    #define ESP_LOG_WARN_COLOR          LOG_COLOR(LOG_COLOR_BROWN)
    #define ESP_LOG_INFO_COLOR          LOG_COLOR(LOG_COLOR_GREEN)
    #define ESP_LOG_DEBUG_COLOR         LOG_COLOR(LOG_COLOR_CYAN)
    #define ESP_LOG_VERBOSE_COLOR

    #define ESP_LOG_ERROR_LETTER        "E"
    #define ESP_LOG_WARN_LETTER         "W"
    #define ESP_LOG_INFO_LETTER         "I"
    #define ESP_LOG_DEBUG_LETTER        "D"
    #define ESP_LOG_VERBOSE_LETTER      "V"

// esp_hw_support
    #define ESP_HW_LOGE(tag, fmt, ...)  ESP_EARLY_LOGE(tag, fmt, ##__VA_ARGS__)
    #define ESP_HW_LOGW(tag, fmt, ...)  ESP_EARLY_LOGW(tag, fmt, ##__VA_ARGS__)
    #define ESP_HW_LOGI(tag, fmt, ...)  ESP_EARLY_LOGI(tag, fmt, ##__VA_ARGS__)
    #define ESP_HW_LOGD(tag, fmt, ...)  ESP_EARLY_LOGD(tag, fmt, ##__VA_ARGS__)
    #define ESP_HW_LOGV(tag, fmt, ...)  ESP_EARLY_LOGV(tag, fmt, ##__VA_ARGS__)

// to be back compatible
    #define LOG_LOCAL_LEVEL             ESP_LOG_INFO
    #define esp_log_buffer_hex(tag, buffer, buff_len)

__BEGIN_DECLS

static inline
    void esp_log_set_level(enum esp_log_level_t level)
    {
        __log_level = level;
    }

extern __attribute__((nothrow))
    uint32_t esp_log_timestamp(void);

static inline
    uint32_t esp_log_early_timestamp(void) { return esp_log_timestamp(); }

extern __attribute__ ((nothrow, format(printf, 1, 2)))
    int esp_log_printf(char const *format, ...);

extern  __attribute__ ((nothrow))
    void esp_log_vprintf(char const *format, va_list arg);

// avoid esp_rom_sys.h
extern __attribute__ ((nothrow, format(printf, 1, 2)))
    int esp_rom_printf(char const *format, ...);

__END_DECLS
#endif
