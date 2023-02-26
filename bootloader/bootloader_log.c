#include "esp_arch.h"
#include "esp_log.h"

#include "uart.h"
#include "sdkconfig.h"

/****************************************************************************
 *  @def
 ****************************************************************************/
enum esp_log_level_t __log_level = CONFIG_BOOTLOADER_LOG_LEVEL;

/****************************************************************************
 *  @implements
*****************************************************************************/
uint32_t esp_log_timestamp(void)
{
    return __get_CCOUNT() / (CONFIG_ESP_DEFAULT_CPU_FREQ_MHZ * 1000);
}
