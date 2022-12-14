#include "esp_log.h"

void esp_log_level_set(char const *tag, esp_log_level_t level)
{
    (void)tag;
    (void)level;
}

esp_log_level_t esp_log_level_get(char const *tag)
{
    (void)tag;
    return ESP_LOG_NONE;
}

uint32_t esp_log_timestamp(void)
{
    return 0;
}

char const *esp_log_system_timestamp(void)
{
    return "";
}

uint32_t esp_log_early_timestamp(void)
{
    return 0;
}

void esp_log_write(esp_log_level_t level, char const *tag, char const *format, ...)
{
    va_list list;
    va_start(list, format);
    esp_log_writev(level, tag, format, list);
    va_end(list);
}

void esp_log_writev(esp_log_level_t level, char const *tag, char const *format, va_list args)
{
    (void)level;
    (void)tag;
    (void)format;
    (void)args;
}
