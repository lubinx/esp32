#pragma once

#include_next <sys/lock.h>

struct __lock
{
    void *hdl;          // SemaphoreHandle_t
    unsigned dummy[12];
};
typedef struct __lock _lock_t;

static inline __attribute((nonnull, nothrow))
    void _lock_init(_LOCK_T lock)
    {
        __retarget_lock_init(&lock);
    }

static inline __attribute((nonnull, nothrow))
    void _lock_init_recursive(_LOCK_T lock)
    {
        __retarget_lock_init_recursive(&lock);
    }

extern __attribute((nonnull, nothrow))
    void _lock_close(_LOCK_T lock);
extern __attribute((nonnull, nothrow))
    void _lock_close_recursive(_LOCK_T lock);

extern __attribute((nonnull, nothrow))
    void _lock_acquire(_LOCK_T lock);
extern __attribute((nonnull, nothrow))
    void _lock_acquire_recursive(_LOCK_T lock);
extern __attribute((nonnull, nothrow))
    int _lock_try_acquire(_LOCK_T lock);
extern __attribute((nonnull, nothrow))
    int _lock_try_acquire_recursive(_LOCK_T lock);

extern __attribute((nonnull, nothrow))
    void _lock_release(_LOCK_T lock);
extern __attribute((nonnull, nothrow))
    void _lock_release_recursive(_LOCK_T lock);
