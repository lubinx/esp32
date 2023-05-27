#include <stdio.h>
#include <reent.h>
#include <sys/types.h>

#include "esp_arch.h"
#include "esp_log.h"

#include "sdkconfig.h"

/****************************************************************************
 *  @def
 ****************************************************************************/
enum __ESP_LOG_t __log_level = CONFIG_ESP_SYSTEM_LOG_LEVEL;

/****************************************************************************
 *  @implements
*****************************************************************************/
uint32_t esp_log_timestamp(void)
{
    // when bootup this is 20 Mhz
    return __get_CCOUNT() / (20 * 1000);
}

int esp_log_printf(char const *format, ...)
{
    va_list va;
    va_start(va, format);

    int retval = vprintf(format, va);
    va_end(va);

    fflush(stdout);
    return retval;
}
