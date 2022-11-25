#include "esp_log.h"

__attribute__ ((weak))
void esp_log_writev(esp_log_level_t level, char const *tag, char const *format, va_list args)
{
    (void)level;
    (void)tag;
    (void)format;
    (void)args;
}
