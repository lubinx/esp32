#pragma once

#include "esp_arch.h"

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