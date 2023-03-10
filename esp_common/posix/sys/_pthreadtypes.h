#ifndef __PTHREAD_TYPES_H
#define __PTHREAD_TYPES_H               1

#include <rtos/types.h>
#include <sys/mutex.h>

/** Used to identify a thread. */
    typedef void *                  pthread_t;

/** Used for dynamic package initialisation. */
    typedef void *                  pthread_once_t;

/** Used to identify a thread attribute object. */
    struct __pthread_attr
    {
        int stacksize;
        int detachstate;
        uint32_t dummy[4];
    };
    typedef struct __pthread_attr   pthread_attr_t;

/** pthread entry type */
    typedef void *(*pthread_routine_t)(void *arg);

/** Used for thread-specific data keys. */
    typedef void *                  pthread_key_t;

/** spinlock. */
    struct __pthread_spinlock
    {
        void *owner;
    };
    typedef struct __pthread_spinlock pthread_spinlock_t;
    #define PTHREAD_SPINLOCK_INITIALIZER    (0)

/** mutexes. */
    typedef struct KERNEL_hdl       pthread_mutex_t;
    #define PTHREAD_MUTEX_INITIALIZER   \
        MUTEX_INITIALIZER
    #define PTHREAD_RECURSIVE_MUTEX_INITIALIZER \
        MUTEX_RECURSIVE_INITIALIZER

    struct __pthread_mutexattr
    {
        int type;
    };
    typedef struct __pthread_mutexattr  pthread_mutexattr_t;

/**  ondition variables. */
    typedef struct semaphore_t      pthread_cond_t;

    struct __pthread_condattr
    {
        uint32_t dummy[1];
    };
    typedef struct __pthread_condattr pthread_condattr_t;

/** read-write locks. */
    typedef struct rwlock_t         pthread_rwlock_t;

    struct __pthread_rwlockattr
    {
        uint32_t dummy[1];
    };
    typedef struct __pthread_rwlockattr pthread_rwlockattr_t;

    #define PTHREAD_RWLOCK_INITIALIZER  \
        RWLOCK_INITIALIZER

#endif
