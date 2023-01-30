#include <signal.h>
#include <stdlib.h>
#include <string.h>

#include "xtensa/xtensa_api.h"
#include "soc/soc_caps.h"
#include "hal/efuse_hal.h"

#include "esp_attr.h"
#include "esp_cpu.h"
#include "esp_heap_caps.h"
#include "esp_rom_sys.h"

#include "esp_system.h"
#include "esp_private/system_internal.h"

void esp_restart(void)
{
    exit(0);
    while (1);
}

void esp_restart_noos(void)
    __attribute__((alias("esp_restart")));

void esp_restart_noos_dig(void)
    __attribute__((alias("esp_restart")));

void esp_system_abort(const char *details)
{
    abort();
}

esp_err_t esp_register_shutdown_handler(void (*function)(void))
{
    return atexit(function);
}

const char *esp_get_idf_version(void)
{
    return IDF_VER;
}

uint32_t esp_get_free_heap_size(void)
{
    return heap_caps_get_free_size(MALLOC_CAP_DEFAULT);
}

uint32_t esp_get_free_internal_heap_size(void)
{
    return heap_caps_get_free_size(MALLOC_CAP_8BIT | MALLOC_CAP_DMA | MALLOC_CAP_INTERNAL);
}

uint32_t esp_get_minimum_free_heap_size(void)
{
    return heap_caps_get_minimum_free_size(MALLOC_CAP_DEFAULT);
}

#include "esp_ipc.h"

esp_err_t esp_ipc_call(uint32_t cpu_id, esp_ipc_func_t func, void* arg)
{
    func(arg);
    return ESP_OK;
}

esp_err_t esp_ipc_call_blocking(uint32_t cpu_id, esp_ipc_func_t func, void* arg)
{
    func(arg);
    return ESP_OK;
}
