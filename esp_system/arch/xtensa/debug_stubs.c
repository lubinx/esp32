/*
 * SPDX-FileCopyrightText: 2017-2021 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

// This module implements debug/trace stubs. The stub is a piece of special code which can invoked by OpenOCD
// Currently one stub is used for GCOV functionality
//

#include "xtensa-debug-module.h"
#include "esp_compiler.h"
#include "esp_log.h"

const static char *TAG = "esp_dbg_stubs";

#define ESP_DBG_STUBS_TRAX_REG              ERI_TRAX_TRIGGERPC

void esp_dbg_stubs_ll_init(void *stub_table)
{
    __WER(ESP_DBG_STUBS_TRAX_REG, (unsigned)stub_table);
    ESP_LOGV(TAG, "%s stubs %x", __func__, __RER(ESP_DBG_STUBS_TRAX_REG));
}
