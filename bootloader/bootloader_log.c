#include "esp_compiler.h"
#include "esp_rom_sys.h"
#include "esp_log.h"

uint32_t esp_log_timestamp(void)
{
    return __get_CCOUNT() / (esp_rom_get_cpu_ticks_per_us() * 1000);
}
