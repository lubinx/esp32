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

// Return global reent struct if any newlib functions are linked to bootloader
struct _reent *__getreent(void)
{
    return _GLOBAL_REENT;
}
