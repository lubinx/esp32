#pragma once

#include_next <sys/lock.h>

typedef _LOCK_T _lock_t;

struct __lock
{
    int __pad[20];
};
typedef struct __lock   sys_static_lock_t;

extern __attribute__((nonnull, nothrow))
    void libc_lock_init(_LOCK_T *lock);
extern __attribute__((nonnull, nothrow))
    void libc_lock_init_recursive(_LOCK_T *lock);

extern __attribute__((nonnull, nothrow))
    void libc_lock_sinit(_LOCK_T lock);
extern __attribute__((nonnull, nothrow))
    void libc__lock_sinit_recursive(_LOCK_T lock);

extern __attribute__((nonnull, nothrow))
    void libc_lock_close(_LOCK_T lock);
extern __attribute__((nonnull, nothrow))
    void libc_lock_close_recursive(_LOCK_T lock);

extern __attribute__((nonnull, nothrow))
    void libc_lock_acquire(_LOCK_T lock);
extern __attribute__((nonnull, nothrow))
    void libc_lock_acquire_recursive(_LOCK_T lock);

extern __attribute__((nonnull, nothrow))
    int libc_lock_try_acquire(_LOCK_T lock);
extern __attribute__((nonnull, nothrow))
    int libc_lock_try_acquire_recursive(_LOCK_T lock);

extern __attribute__((nonnull, nothrow))
    void libc_lock_release(_LOCK_T lock);
extern __attribute__((nonnull, nothrow))
    void libc_lock_release_recursive(_LOCK_T lock);

/***************************************************************************/
/** esp-idf
****************************************************************************/
static inline
    void _lock_init(_LOCK_T *lock)
    {
        libc_lock_init(lock);
    }

static inline
    void _lock_init_recursive(_LOCK_T *lock)
    {
        libc_lock_init_recursive(lock);
    }

static inline
    void _lock_close(_LOCK_T *lock)
    {
        libc_lock_close(*lock);
    }

static inline
    void _lock_close_recursive(_LOCK_T *lock)
    {
        libc_lock_close_recursive(*lock);
    }

extern __attribute__((nonnull, nothrow))
    int __esp_lock_impl(_LOCK_T *lock, int (*libc_lock_func)(_LOCK_T lock), char const *__function__);

static inline
    void _lock_acquire(_LOCK_T *lock)
    {
        __esp_lock_impl(lock, (int (*)(_LOCK_T))libc_lock_acquire, __func__);
    }

static inline
    void _lock_acquire_recursive(_LOCK_T *lock)
    {
        __esp_lock_impl(lock, (int (*)(_LOCK_T))libc_lock_acquire_recursive, __func__);
    }

static inline
    int _lock_try_acquire(_LOCK_T *lock)
    {
        return __esp_lock_impl(lock, libc_lock_try_acquire, __func__);
    }

static inline
    int _lock_try_acquire_recursive(_LOCK_T *lock)
    {
        return __esp_lock_impl(lock, libc_lock_try_acquire_recursive, __func__);
    }

static inline
    void _lock_release(_LOCK_T *lock)
    {
        __esp_lock_impl(lock, (int (*)(_LOCK_T))libc_lock_release, __func__);
    }

static inline
    void _lock_release_recursive(_LOCK_T *lock)
    {
        __esp_lock_impl(lock, (int (*)(_LOCK_T))libc_lock_release, __func__);
    }
