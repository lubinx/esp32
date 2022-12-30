#include "esp_cpu.h"
#include "esp_rom_sys.h"
#include "esp_log.h"

uint32_t esp_log_timestamp(void)
{
    return esp_cpu_get_cycle_count() / (esp_rom_get_cpu_ticks_per_us() * 1000);
}
