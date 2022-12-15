#include <stdio.h>
#include "esp_log.h"

void esp_log_write(esp_log_level_t level, char const *tag, char const *format, ...)
{
    (void)level;
    (void)tag;
    (void)format;
}
