#include <stdio.h>
#include <sys/reent.h>

#include "esp_arch.h"
#include "esp_log.h"

#include "sdkconfig.h"

/****************************************************************************
 *  @def
 ****************************************************************************/
enum __ESP_LOG_t __log_level = CONFIG_ESP_SYSTEM_LOG_LEVEL;

/****************************************************************************
 *  @implements
*********************************************************p********************/
__attribute__((weak))
uint32_t esp_log_timestamp(void)
{
    return __get_CCOUNT() / (CONFIG_ESP_DEFAULT_CPU_FREQ_MHZ * 1000);
}

/*
__attribute__((weak))
int esp_log_printf(char const *format, ...)
{
    return 0;
}
*/
