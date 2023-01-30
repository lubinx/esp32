/*
 * SPDX-FileCopyrightText: 2015-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <features.h>
#include <sys/reent.h>

__BEGIN_DECLS

extern __attribute__((nothrow))
    void esp_newlib_init(void);

extern __attribute__((nothrow))
    void esp_reent_init(struct _reent *r);

/**
 * Update current microsecond time from RTC
 */
void esp_set_time_from_rtc(void);

/*
 * Sync timekeeping timers, RTC and high-resolution timer. Update boot_time.
 */
void esp_sync_timekeeping_timers(void);

__END_DECLS
