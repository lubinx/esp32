#include <sys/types.h>

#include "esp_arch.h"
#include "esp_log.h"

#include "uart.h"
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
