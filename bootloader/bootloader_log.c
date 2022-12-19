#include "esp_cpu.h"
#include "esp_rom_sys.h"
#include "esp_log.h"

uint32_t esp_log_timestamp(void)
{
    return esp_cpu_get_cycle_count() / (esp_rom_get_cpu_ticks_per_us() * 1000);
}

void esp_log_write(esp_log_level_t level, char const *tag, char const *format, ...)
{
    (void)level;
    (void)tag;
    (void)format;
}
