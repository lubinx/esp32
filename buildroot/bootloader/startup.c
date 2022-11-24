/*
 * SPDX-FileCopyrightText: 2015-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <stdbool.h>

#include "esp_log.h"
#include "bootloader_init.h"
#include "bootloader_utility.h"
#include "esp_rom_sys.h"

void __attribute__((noreturn, weak)) call_start_cpu0(void)
{
    if (ESP_OK != bootloader_init())
        bootloader_reset();

    // printf("esp_rom_printf\n");

    while (1)
    {
    };
}

__attribute__((weak))
struct _reent *__getreent(void)
{
    return _GLOBAL_REENT;
}

int esp_app_get_elf_sha256(char* dst, size_t size)
{
    return 0;
}
