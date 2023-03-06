/*
 * SPDX-FileCopyrightText: 2015-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#pragma once

#include <sys/cdefs.h>
#include <stdbool.h>

#include "xt_utils.h"

    struct xt_spinlock_t
    {
        unsigned volatile core_id;
        unsigned lock_count;
    };
    typedef struct xt_spinlock_t    spinlock_t;

__BEGIN_DECLS

static inline __attribute__((nonnull, nothrow))
    void spin_lock_init(spinlock_t *lock)
    {
        lock->core_id = 0;
        lock->lock_count = 0;
    }

static inline
    bool spin_is_locked(spinlock_t *lock)
    {
        return 0 != lock->core_id;
    }

static inline __attribute__((nonnull, nothrow))
    void spin_lock(spinlock_t *lock)
    {
        unsigned core_id = 1U << __get_CORE_ID();

        if (lock->core_id != core_id)
        {
            while (! __sync_bool_compare_and_swap(&lock->core_id, 0, core_id));
            lock->lock_count ++;
        }
        else
            lock->lock_count ++;
    }

static inline __attribute__((nonnull, nothrow))
    void spin_unlock(spinlock_t *lock)
    {
        assert(lock->core_id == (1U << __get_CORE_ID()));

        if (0 == -- lock->lock_count)
            lock->core_id = 0;
    }

// for esp-idf compatiable
    #define SPINLOCK_INITIALIZER        {.core_id = 0, .lock_count = 0}
    #define SPINLOCK_WAIT_FOREVER       ((unsigned)(-1))

static inline
    void spinlock_initialize(spinlock_t *lock) __attribute__((alias("spin_lock_init")));

static inline
    bool spinlock_acquire(spinlock_t *lock, unsigned timeout)
    {
        assert(timeout == SPINLOCK_WAIT_FOREVER);
        spin_lock(lock);
        return true;
    }

static inline
    void spinlock_release(spinlock_t *lock) __attribute__((alias("spin_unlock")));

__END_DECLS