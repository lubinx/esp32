/*
 * SPDX-FileCopyrightText: 2015-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#pragma once

#include <sys/cdefs.h>
#include <stdbool.h>
#include <stdint.h>

#include "xt_utils.h"

    struct xt_spinlock_t
    {
        unsigned volatile core_id;
        unsigned lock_count;
        unsigned irq_status;
    };
    typedef struct xt_spinlock_t    spinlock_t;

    #define SPINLOCK_INITIALIZER        {.core_id = 0, .lock_count = 0}

__BEGIN_DECLS

static inline __attribute__((nonnull, nothrow))
    void spinlock_init(spinlock_t *lock)
    {
        lock->core_id = 0;
        lock->lock_count = 0;
    }

static inline __attribute__((nonnull, nothrow))
    void spin_lock_init(spinlock_t *lock) __attribute((alias("spinlock_init")));

static inline __attribute__((nonnull, nothrow))
    void spin_lock(spinlock_t *lock)
    {
        uint32_t irq_status = XTOS_SET_INTLEVEL(XCHAL_EXCM_LEVEL);
        unsigned core_id = 1U << __get_CORE_ID();

        if (lock->core_id != core_id)
        {
            while (! __sync_bool_compare_and_swap(&lock->core_id, 0, core_id));

            lock->lock_count ++;
            lock->irq_status = irq_status;
        }
        else
            lock->lock_count ++;
    }

static inline __attribute__((nonnull, nothrow))
    void spin_unlock(spinlock_t *lock)
    {
        assert(lock->core_id == (1U << __get_CORE_ID()));

        if (0 == __sync_sub_and_fetch(&lock->lock_count, 1))
        {
            lock->core_id = 0;
            XTOS_RESTORE_INTLEVEL(lock->irq_status);
        }
    }

// for esp-idf compatiable
    #define SPINLOCK_WAIT_FOREVER       (~0)

static inline __attribute__((nonnull, nothrow))
    bool spinlock_acquire(spinlock_t *lock, unsigned timeout)
    {
        assert(timeout == (unsigned)SPINLOCK_WAIT_FOREVER);
        spin_lock(lock);
        return true;
    }

static inline __attribute__((nonnull, nothrow))
    void spinlock_release(spinlock_t *lock) __attribute__((alias("spin_unlock")));

__END_DECLS
