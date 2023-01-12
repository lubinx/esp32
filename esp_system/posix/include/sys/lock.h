#pragma once

#include_next <sys/lock.h>

struct __lock
{
    int __dummy[20];
};
typedef struct __lock   sys_static_lock_t;

/**
 *  lock static initialize
*/
extern __attribute__((nonnull, nothrow))
    void _static_lock_init(_LOCK_T lock);
extern __attribute__((nonnull, nothrow))
    void _static_lock_init_recursive(_LOCK_T lock);

/***************************************************************************/
/** esp-idf
****************************************************************************/
typedef _LOCK_T _lock_t;

extern __attribute__((nonnull, nothrow))
    void _lock_init(_lock_t *lock);
extern __attribute__((nonnull, nothrow))
    void _lock_init_recursive(_lock_t *lock);

extern __attribute__((nonnull, nothrow))
    void _lock_close(_lock_t *lock);
extern __attribute__((nonnull, nothrow))
    void _lock_close_recursive(_lock_t *lock);

extern __attribute__((nonnull, nothrow))
    void _lock_acquire(_lock_t *lock);
extern __attribute__((nonnull, nothrow))
    void _lock_acquire_recursive(_lock_t *lock);

extern __attribute__((nonnull, nothrow))
    int _lock_try_acquire(_lock_t *lock);
extern __attribute__((nonnull, nothrow))
    int _lock_try_acquire_recursive(_lock_t *lock);

extern __attribute__((nonnull, nothrow))
    void _lock_release(_lock_t *lock);
extern __attribute__((nonnull, nothrow))
    void _lock_release_recursive(_lock_t *lock);
