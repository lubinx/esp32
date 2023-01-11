/*
 * SPDX-FileCopyrightText: 2015-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#pragma once

#include <features.h>
#include <stdbool.h>
#include <stdint.h>

    #define SPINLOCK_INITIALIZER        {.cpuid = 0, .intr_status = 0, .lock_count = 0}
    #define SPINLOCK_WAIT_FOREVER       ((uint32_t)(-1))
    #define SPINLOCK_NO_WAIT            0U

    typedef struct
    {
        uint32_t cpuid;
        uint32_t intr_status;

        uint32_t lock_count;
    } spinlock_t;

__BEGIN_DECLS

extern __attribute__((nonnull, nothrow))
    void spinlock_initialize(spinlock_t *lock);

extern __attribute__((nonnull, nothrow))
    bool spinlock_acquire(spinlock_t *lock, uint32_t timeout);

extern __attribute__((nonnull, nothrow))
    void spinlock_release(spinlock_t *lock);

__END_DECLS
