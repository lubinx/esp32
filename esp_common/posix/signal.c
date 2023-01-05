#include <unistd.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/errno.h>

#include "esp_attr.h"
#include "esp_cpu.h"
#include "esp_log.h"
#include "esp_rom_sys.h"

void __attribute__((noreturn)) __assert_func(const char *file, int line, const char *func, const char *failedexpr)
{
    ESP_LOGE("assertion", "\"%s\" failed\n\tfile \"%s\", line %d%s%s\n",
        failedexpr, file, line, func ? ", function: " : "", func ? func : "");

    if (esp_cpu_dbgr_is_attached())
        esp_cpu_dbgr_break();

    exit(EFAULT);
}

void abort(void)
{
    ESP_LOGE("signal", "abort() was called at PC 0x%p on core %d", __builtin_return_address(0) - 3, esp_cpu_get_core_id());

    if (esp_cpu_dbgr_is_attached())
        esp_cpu_dbgr_break();

    exit(EFAULT);
}

void IRAM_ATTR _exit(int status)
{
    #ifdef CONFIG_IDF_TARGET_ARCH_RISCV
        rv_utils_intr_global_disable();
    #else
        xt_ints_off(0xFFFFFFFF);
    #endif

    int core_id = esp_cpu_get_core_id();
    for (uint32_t i = 0; i < SOC_CPU_CORES_NUM; i ++)
    {
        if (i != core_id)
            esp_cpu_unstall(i);
    }

    esp_rom_software_reset_system();
    while (1);
}

int atexit(void (*function)(void))
{
    return 0;
}
