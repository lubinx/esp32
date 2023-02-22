/*
 * SPDX-FileCopyrightText: 2016-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#ifdef __XTENSA__
    #include "xt_utils.h"
#elif __riscv
    #include "riscv/rv_utils.h"
#endif

    // debugger
    #define __dbgr_is_attached()        xt_utils_dbgr_is_attached()
    #define __dbgr_break()              xt_utils_dbgr_break()
    #define __BKPT(value)               (__dbgr_is_attached() ? __dbgr_break(): (void)value)

/*
 * The likely and unlikely macro pairs:
 * These macros are useful to place when application
 * knows the majority ocurrence of a decision paths,
 * placing one of these macros can hint the compiler
 * to reorder instructions producing more optimized
 * code.
 */
/*
#ifndef likely
    #define likely(x)      __builtin_expect(!!(x), 1)
#endif
#ifndef unlikely
    #define unlikely(x)    __builtin_expect(!!(x), 0)
#endif
*/

/*
 * Utility macros used for designated initializers, which work differently
 * in C99 and C++ standards mainly for aggregate types.
 * The member separator, comma, is already part of the macro, please omit the trailing comma.
 * Usage example:
 *   struct config_t { char* pchr; char arr[SIZE]; } config = {
 *              ESP_COMPILER_DESIGNATED_INIT_AGGREGATE_TYPE_EMPTY(pchr)
 *              ESP_COMPILER_DESIGNATED_INIT_AGGREGATE_TYPE_STR(arr, "Value")
 *          };
 */
// REVIEW: only used in esp_wps.h
#if defined(__cplusplus) && __cplusplus >= 202002L
    #define ESP_COMPILER_DESIGNATED_INIT_AGGREGATE_TYPE_STR(member, value)  .member = value,
    #define ESP_COMPILER_DESIGNATED_INIT_AGGREGATE_TYPE_EMPTY(member) .member = { },
#elif defined(__cplusplus) && __cplusplus < 202002L
    #define ESP_COMPILER_DESIGNATED_INIT_AGGREGATE_TYPE_STR(member, value)  { .member = value },
    #define ESP_COMPILER_DESIGNATED_INIT_AGGREGATE_TYPE_EMPTY(member) .member = { },
#else
    #define ESP_COMPILER_DESIGNATED_INIT_AGGREGATE_TYPE_STR(member, value)  .member = value,
    #define ESP_COMPILER_DESIGNATED_INIT_AGGREGATE_TYPE_EMPTY(member)
#endif
