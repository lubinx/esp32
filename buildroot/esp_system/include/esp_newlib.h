/*
 * SPDX-FileCopyrightText: 2015-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <features.h>
#include <sys/reent.h>

__BEGIN_DECLS
    /* Kept for backward compatibility */
    #define esp_sync_counters_rtc_and_frc esp_sync_timekeeping_timers

    /**
     * Function which sets up newlib in ROM for use with ESP-IDF
     *
     * Includes defining the syscall table, setting up any common locks, etc.
     *
     * Called from the startup code, not intended to be called from application
     * code.
     */
static inline
    void esp_newlib_init(void)
    {
    }

   /*
    * Initialize newlib time functions
    */
extern __attribute__((nothrow))
    void esp_newlib_time_init(void);

    /**
     * Replacement for newlib's _REENT_INIT_PTR and __sinit.
     *
     * Called from startup code and FreeRTOS, not intended to be called from
     * application code.
     */
extern __attribute__((nothrow))
    void esp_reent_init(struct _reent* r);

    /**
     * Clean up some of lazily allocated buffers in REENT structures.
     */
extern __attribute__((nothrow))
    void esp_reent_cleanup(void);

    /**
     * Update current microsecond time from RTC
     */
extern __attribute__((nothrow))
    void esp_set_time_from_rtc(void);

    /*
    * Sync timekeeping timers, RTC and high-resolution timer. Update boot_time.
    */
extern __attribute__((nothrow))
    void esp_sync_timekeeping_timers(void);

/**
 * Initialize newlib static locks
 */
void esp_newlib_locks_init(void);

__END_DECLS
