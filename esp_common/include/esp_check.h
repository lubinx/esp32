/*
 * SPDX-FileCopyrightText: 2021-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#pragma once

#include "esp_err.h"

/*
 * The likely and unlikely macro pairs:
 * These macros are useful to place when application
 * knows the majority ocurrence of a decision paths,
 * placing one of these macros can hint the compiler
 * to reorder instructions producing more optimized
 * code.
 */
#ifndef likely
    #define likely(x)                   __builtin_expect(!!(x), 1)
#endif
#ifndef unlikely
    #define unlikely(x)                 __builtin_expect(!!(x), 0)
#endif

/**
 * Macro which can be used to check the error code. If the code is not ESP_OK, it prints the message and returns.
 */
#define ESP_RETURN_ON_ERROR(x, log_tag, format, ...) do {                                       \
        (void)log_tag;                                                                          \
        esp_err_t err_rc_ = (x);                                                                \
        if (unlikely(ESP_OK != err_rc_)) {                                                      \
            return err_rc_;                                                                     \
        }                                                                                       \
    } while(0)

/**
 * A version of ESP_RETURN_ON_ERROR() macro that can be called from ISR.
 */
#define ESP_RETURN_ON_ERROR_ISR(x, log_tag, format, ...) do {                                   \
        (void)log_tag;                                                                          \
        esp_err_t err_rc_ = (x);                                                                \
        if (unlikely(ESP_OK != err_rc_)) {                                                      \
            return err_rc_;                                                                     \
        }                                                                                       \
    } while(0)

/**
 * Macro which can be used to check the error code. If the code is not ESP_OK, it prints the message,
 * sets the local variable 'ret' to the code, and then exits by jumping to 'goto_tag'.
 */
#define ESP_GOTO_ON_ERROR(x, goto_tag, log_tag, format, ...) do {                               \
        (void)log_tag;                                                                          \
        esp_err_t err_rc_ = (x);                                                                \
        if (unlikely(ESP_OK != err_rc_)) {                                                      \
            ret = err_rc_;                                                                      \
            goto goto_tag;                                                                      \
        }                                                                                       \
    } while(0)

/**
 * A version of ESP_GOTO_ON_ERROR() macro that can be called from ISR.
 */
#define ESP_GOTO_ON_ERROR_ISR(x, goto_tag, log_tag, format, ...) do {                           \
        (void)log_tag;                                                                          \
        esp_err_t err_rc_ = (x);                                                                \
        if (unlikely(ESP_OK != err_rc_)) {                                                      \
            ret = err_rc_;                                                                      \
            goto goto_tag;                                                                      \
        }                                                                                       \
    } while(0)

/**
 * Macro which can be used to check the condition. If the condition is not 'true', it prints the message
 * and returns with the supplied 'err_code'.
 */
#define ESP_RETURN_ON_FALSE(a, err_code, log_tag, format, ...) do {                             \
        (void)log_tag;                                                                          \
        if (unlikely(!(a))) {                                                                   \
            return err_code;                                                                    \
        }                                                                                       \
    } while(0)

/**
 * A version of ESP_RETURN_ON_FALSE() macro that can be called from ISR.
 */
#define ESP_RETURN_ON_FALSE_ISR(a, err_code, log_tag, format, ...) do {                         \
        (void)log_tag;                                                                          \
        if (unlikely(!(a))) {                                                                   \
            return err_code;                                                                    \
        }                                                                                       \
    } while(0)

/**
 * Macro which can be used to check the condition. If the condition is not 'true', it prints the message,
 * sets the local variable 'ret' to the supplied 'err_code', and then exits by jumping to 'goto_tag'.
 */
#define ESP_GOTO_ON_FALSE(a, err_code, goto_tag, log_tag, format, ...) do {                     \
        (void)log_tag;                                                                          \
        if (unlikely(!(a))) {                                                                   \
            ret = err_code;                                                                     \
            goto goto_tag;                                                                      \
        }                                                                                       \
    } while (0)

/**
 * A version of ESP_GOTO_ON_FALSE() macro that can be called from ISR.
 */
#define ESP_GOTO_ON_FALSE_ISR(a, err_code, goto_tag, log_tag, format, ...) do {                 \
        (void)log_tag;                                                                          \
        if (unlikely(!(a))) {                                                                   \
            ret = err_code;                                                                     \
            goto goto_tag;                                                                      \
        }                                                                                       \
    } while (0)
#endif
