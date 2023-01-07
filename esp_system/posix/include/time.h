/*
 * SPDX-FileCopyrightText: 2018-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <features.h>
#include <sys/types.h>
#include_next <time.h>

#ifndef CLOCK_MONOTONIC
    #define CLOCK_MONOTONIC (clockid_t)4
#endif

#ifndef CLOCK_BOOTTIME
    #define CLOCK_BOOTTIME (clockid_t)4
#endif

__BEGIN_DECLS

    // int clock_settime(clockid_t clock_id, const struct timespec *tp);
    // int clock_gettime(clockid_t clock_id, struct timespec *tp);
    // int clock_getres(clockid_t clock_id, struct timespec *res);

__END_DECLS
