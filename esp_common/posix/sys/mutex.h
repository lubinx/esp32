#ifndef __MUTEX_H
#define __MUTEX_H                       1

#include <features.h>
#include <sys/lock.h>

#include "rtos/types.h"

__BEGIN_DECLS

extern __attribute__((nonnull, nothrow))
    mutex_t *mutex_create(int flags);

extern __attribute__((nonnull, nothrow))
    int mutex_destroy(mutex_t *mutex);

extern __attribute__((nonnull, nothrow))
    int mutex_init(mutex_t *mutex, int flags);

extern __attribute__((nonnull, nothrow))
    int mutex_lock(mutex_t *mutex);

extern __attribute__((nonnull, nothrow))
    int mutex_unlock(mutex_t *mutex);

extern __attribute__((nonnull, nothrow))
    int mutex_trylock(mutex_t *mutex, uint32_t timeout);

__END_DECLS
#endif
