/*
 * SPDX-FileCopyrightText: 2015-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#pragma once

#include <features.h>
#include <stdbool.h>
#include <stdint.h>

#if defined(__riscv)
    #define SPINLOCK_INITIALIZER        {0}

    typedef struct
    {
        uint32_t count;
    } spinlock_t;

static inline
    void spinlock_initialize(spinlock_t *lock)
    {
        lock->count = 0;
    }

static inline
    bool spinlock_acquire(spinlock_t *lock, uint32_t timeout)
    {
        lock->count ++;
        return true;
    }

static inline
    void spinlock_release(spinlock_t *lock)
    {
        lock->count --;
    }

#elif defined(__XTENSA__)
    #include "xtensa/xtruntime.h"
    #include "xt_utils.h"
    #include "esp_cpu.h"

    #define SPINLOCK_INITIALIZER        {.owner = SPINLOCK_FREE,.count = 0}

    #define SPINLOCK_FREE               0xB33FFFFF
    #define SPINLOCK_WAIT_FOREVER       ((uint32_t)(-1))
    #define SPINLOCK_NO_WAIT            0U
    #define CORE_ID_REGVAL_XOR_SWAP     (0xCDCD ^ 0xABAB)

    typedef struct
    {
        uint32_t owner;
        uint32_t count;
    } spinlock_t;

__BEGIN_DECLS

static inline
    void spinlock_initialize(spinlock_t *lock)
    {
        lock->owner = SPINLOCK_FREE;
        lock->count = 0;
    }

static inline
    bool spinlock_acquire(spinlock_t *lock, uint32_t timeout)
    {
        uint32_t irq_status;
        uint32_t core_id, other_core_id;
        bool lock_set;
        esp_cpu_cycle_count_t start_count;

        irq_status = XTOS_SET_INTLEVEL(XCHAL_EXCM_LEVEL);

        // Note: The core IDs are the full 32 bit (CORE_ID_REGVAL_PRO/CORE_ID_REGVAL_APP) values
        core_id = xt_utils_get_raw_core_id();
        other_core_id = CORE_ID_REGVAL_XOR_SWAP ^ core_id;

        /* lock->owner should be one of SPINLOCK_FREE, CORE_ID_REGVAL_PRO,
        * CORE_ID_REGVAL_APP:
        *  - If SPINLOCK_FREE, we want to atomically set to 'core_id'.
        *  - If "our" core_id, we can drop through immediately.
        *  - If "other_core_id", we spin here.
        */

        // The caller is already the owner of the lock. Simply increment the nesting count
        if (lock->owner == core_id) {
            assert(lock->count > 0 && lock->count < 0xFF);    // Bad count value implies memory corruption
            lock->count++;
            XTOS_RESTORE_INTLEVEL(irq_status);
            return true;
        }
        /* First attempt to take the lock.
        *
        * Note: We do a first attempt separately (instead of putting this into a loop) in order to avoid call to
        * esp_cpu_get_cycle_count(). This doing a first attempt separately makes acquiring a free lock quicker, which
        * is the case for the majority of spinlock_acquire() calls (as spinlocks are free most of the time since they
        * aren't meant to be held for long).
        */
        lock_set = esp_cpu_compare_and_set(&lock->owner, SPINLOCK_FREE, core_id);
        if (lock_set || timeout == SPINLOCK_NO_WAIT) {
            // We've successfully taken the lock, or we are not retrying
            goto exit;
        }

        // First attempt to take the lock has failed. Retry until the lock is taken, or until we timeout.
        start_count = esp_cpu_get_cycle_count();
        do {
            lock_set = esp_cpu_compare_and_set(&lock->owner, SPINLOCK_FREE, core_id);
            if (lock_set) {
                break;
            }
            // Keep looping if we are waiting forever, or check if we have timed out
        } while ((timeout == SPINLOCK_WAIT_FOREVER) || (esp_cpu_get_cycle_count() - start_count) <= timeout);

    exit:
        if (lock_set) {
            assert(lock->owner == core_id);
            assert(lock->count == 0);   // This is the first time the lock is set, so count should still be 0
            lock->count++;  // Finally, we increment the lock count
        } else {    // We timed out waiting for lock
            assert(lock->owner == SPINLOCK_FREE || lock->owner == other_core_id);
            assert(lock->count < 0xFF); // Bad count value implies memory corruption
        }

        XTOS_RESTORE_INTLEVEL(irq_status);
        return lock_set;
    }

static inline
    void spinlock_release(spinlock_t *lock)
    {
        uint32_t irq_status;
        uint32_t core_id;

        assert(lock);
        irq_status = XTOS_SET_INTLEVEL(XCHAL_EXCM_LEVEL);

        core_id = xt_utils_get_raw_core_id();
        assert(core_id == lock->owner); // This is a lock that we didn't acquire, or the lock is corrupt
        lock->count--;

        if (!lock->count) { // If this is the last recursive release of the lock, mark the lock as free
            lock->owner = SPINLOCK_FREE;
        } else {
            assert(lock->count < 0x100); // Indicates memory corruption
        }

        XTOS_RESTORE_INTLEVEL(irq_status);
    }
#endif

__END_DECLS
