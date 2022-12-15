#include "esp_log.h"

/****************************************************************************
 *  export
*****************************************************************************/
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
    (void)level;
    (void)tag;
    (void)format;
}
