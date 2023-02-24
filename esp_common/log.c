#include <stdio.h>
#include <sys/reent.h>

#include "esp_arch.h"
#include "esp_log.h"

#include "sdkconfig.h"

/****************************************************************************
 *  @def
 ****************************************************************************/
enum esp_log_level_t __log_level = LOG_LOCAL_LEVEL;

/****************************************************************************
 *  @implements
*****************************************************************************/
__attribute__((weak))
uint32_t esp_log_timestamp(void)
{
    return __get_CCOUNT() / (CONFIG_ESP_DEFAULT_CPU_FREQ_MHZ * 1000);
}

__attribute__((weak))
int esp_log_printf(char const *format, ...)
{
    (void)format;
    return 0;
}
