/*
 * SPDX-FileCopyrightText: 2015-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <stdbool.h>

#include "esp_log.h"
#include "bootloader_init.h"
#include "bootloader_utility.h"

void __attribute__((noreturn, weak)) call_start_cpu0(void)
{
    if (ESP_OK != bootloader_init())
        bootloader_reset();

    while (1);
}

int esp_app_get_elf_sha256(char* dst, size_t size)
{
    return 0;
}

__attribute__((weak))
void uart_hal_write_txfifo(void *hal, const uint8_t *buf, uint32_t data_size, uint32_t *write_size)
{
}
