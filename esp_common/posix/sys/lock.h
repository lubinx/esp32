#ifndef __SYS_LOCK_H
#define __SYS_LOCK_H                    1

#include <features.h>
#include_next <sys/lock.h>

__BEGIN_DECLS
    typedef _LOCK_T _lock_t;
    struct __lock;

static inline
    void _lock_init(_LOCK_T *lock)
    {
        __retarget_lock_init(lock);
    }

static inline
    void _lock_init_recursive(_LOCK_T *lock)
    {
        __retarget_lock_init_recursive(lock);
    }

static inline
    void _lock_close(_LOCK_T *lock)
    {
        __retarget_lock_close(*lock);
        *lock = NULL;
    }

static inline
    void _lock_close_recursive(_LOCK_T *lock)
    {
        __retarget_lock_close_recursive(*lock);
        *lock = NULL;
    }

extern __attribute__((nonnull, nothrow))
    void _lock_acquire(_LOCK_T *lock);

extern __attribute__((nonnull, nothrow))
    void _lock_acquire_recursive(_LOCK_T *lock);

extern __attribute__((nonnull, nothrow))
    int _lock_try_acquire(_LOCK_T *lock);

extern __attribute__((nonnull, nothrow))
    int _lock_try_acquire_recursive(_LOCK_T *lock);

extern __attribute__((nonnull, nothrow))
    void _lock_release(_LOCK_T *lock);

extern __attribute__((nonnull, nothrow))
    void _lock_release_recursive(_LOCK_T *lock);


__END_DECLS
#endif
