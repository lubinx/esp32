#include "esp_log.h"

__attribute__ ((weak))
void esp_log_write(esp_log_level_t level, char const *tag, char const *format, ...)
{
    va_list list;
    va_start(list, format);
    esp_log_writev(level, tag, format, list);
    va_end(list);
}

__attribute__ ((weak))
void esp_log_writev(esp_log_level_t level, char const *tag, char const *format, va_list args)
{
    (void)level;
    (void)tag;
    (void)format;
    (void)args;
}
