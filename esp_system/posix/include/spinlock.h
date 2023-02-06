/*
 * SPDX-FileCopyrightText: 2015-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#pragma once

#include <features.h>
#include <stdbool.h>
#include <stdint.h>
#include "esp_cpu.h"

#if defined(__XTENSA__)
    #include "xtensa/xtruntime.h"
    #include "xt_utils.h"
#elif defined(__riscv)
#else
    #pragma GCC error "pthread: unsupported arch"
#endif

    #define SPINLOCK_INITIALIZER        {.core_id = 0, .lock_count = 0}
    #define SPINLOCK_WAIT_FOREVER       ((uint32_t)(-1))

    typedef struct
    {
        uint32_t core_id;
        uint32_t lock_count;
    } spinlock_t;

__BEGIN_DECLS

static inline __attribute__((nonnull, nothrow))
    void spinlock_initialize(spinlock_t *lock)
    {
        lock->core_id = 0;
        lock->lock_count = 0;
    }

static inline __attribute__((nonnull, nothrow))
    bool spinlock_acquire(spinlock_t *lock, uint32_t timeout)
    {
        bool lock_set;

        #ifdef __XTENSA__
            uint32_t irq_status = XTOS_SET_INTLEVEL(XCHAL_EXCM_LEVEL);
            uint32_t core_id = 0xFFFF ^ xt_utils_get_raw_core_id();

            // The caller is already the owner of the lock. Simply increment the nesting count
            if (lock->core_id == core_id)
            {
                lock->lock_count ++;
                XTOS_RESTORE_INTLEVEL(irq_status);
                return true;
            }
        #endif

        // lock / trylock will be only 0 or FOREVER
        if (0 != timeout)
        {
            while (! esp_cpu_compare_and_set(&lock->core_id, 0, core_id));
            lock_set = true;
        }
        else
            lock_set = esp_cpu_compare_and_set(&lock->core_id, 0, core_id);

        if (lock_set)
            lock->lock_count ++;

        #ifdef __XTENSA__
            XTOS_RESTORE_INTLEVEL(irq_status);
        #endif

        return lock_set;
    }

static inline __attribute__((nonnull, nothrow))
    void spinlock_release(spinlock_t *lock)
    {
        assert(lock->core_id == (0xFFFF ^ xt_utils_get_raw_core_id()));

        #ifdef __XTENSA__
            uint32_t irq_status = XTOS_SET_INTLEVEL(XCHAL_EXCM_LEVEL);
        #endif

        if (0 == -- lock->lock_count)
            lock->core_id = 0;

        #ifdef __XTENSA__
            XTOS_RESTORE_INTLEVEL(irq_status);
        #endif
    }

__END_DECLS
