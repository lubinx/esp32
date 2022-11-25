/*
 * SPDX-FileCopyrightText: 2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#pragma once

#include <features.h>
#include_next <sys/lock.h>

__BEGIN_DECLS

    /* Actual platfrom-specific definition of struct __lock.
    * The size here should be sufficient for a FreeRTOS mutex.
    * This is checked by a static assertion in locks.c
    *
    * Note 1: this might need to be made dependent on whether FreeRTOS
    * is included in the build.
    *
    * Note 2: the size is made sufficient for the case when
    * configUSE_TRACE_FACILITY is enabled. If it is disabled,
    * this definition wastes 8 bytes.
    */
    struct __lock
    {
        int reserved[23];
    };
    typedef _LOCK_T _lock_t;

    void _lock_init(_lock_t *plock);
    void _lock_init_recursive(_lock_t *plock);
    void _lock_close(_lock_t *plock);
    void _lock_close_recursive(_lock_t *plock);
    void _lock_acquire(_lock_t *plock);
    void _lock_acquire_recursive(_lock_t *plock);
    int _lock_try_acquire(_lock_t *plock);
    int _lock_try_acquire_recursive(_lock_t *plock);
    void _lock_release(_lock_t *plock);
    void _lock_release_recursive(_lock_t *plock);
__END_DECLS
