#include "esp_cpu.h"
#include "esp_rom_sys.h"
#include "esp_log.h"

/****************************************************************************
 *  export
*****************************************************************************/
uint32_t esp_log_early_timestamp(void)
{
    return esp_cpu_get_cycle_count() / (esp_rom_get_cpu_ticks_per_us() * 1000);
}

__attribute__((weak, alias("esp_log_early_timestamp")))
uint32_t esp_log_timestamp(void);

void esp_log_write(esp_log_level_t level, char const *tag, char const *format, ...)
{
    (void)level;
    (void)tag;
    (void)format;
}
