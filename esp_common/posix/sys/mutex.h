#ifndef __MUTEX_H
#define __MUTEX_H                       1

#include <features.h>
#include <sys/lock.h>

    struct __mutex_t
    {
        int __pad[20];
        int __init;
    };
    typedef struct __mutex_t        mutex_t;

    // mutex_create() with flag "no recurse" otherwise is recursive
    #define MUTEX_FLAG_NORMAL           (0x00)
    #define MUTEX_FLAG_RECURSIVE        (0x01)

    // mutex initializer
    #define MUTEX_INITIALIZER           \
        {.__init = ~MUTEX_FLAG_NORMAL}
    #define PTHREAD_RECURSIVE_MUTEX_INITIALIZER \
        {.__init = ~MUTEX_FLAG_RECURSIVE}

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
