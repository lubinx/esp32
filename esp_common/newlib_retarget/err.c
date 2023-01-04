#include "esp_attr.h"

#include "esp_cpu.h"
#include "esp_log.h"

static char const *TAG = "newlib error";

void __attribute__((noreturn)) abort(void)
{
    ESP_LOGE(TAG, "abort() was called at PC 0x%p on core %d", __builtin_return_address(0) - 3, esp_cpu_get_core_id());
    while (1);
}

void __attribute__((noreturn)) __assert_func(const char *file, int line, const char *func, const char *failedexpr)
{
    ESP_LOGE(TAG, "assertion \"%s\" failed: file \"%s\", line %d%s%s\n",
        failedexpr, file, line, func ? ", function: " : "", func ? func : "");
    while (1);
}

// s**t
void _esp_error_check_failed(esp_err_t rc, const char *file, int line, const char *function, const char *expression)
{
    __assert_func(file, line, function, expression);
}
