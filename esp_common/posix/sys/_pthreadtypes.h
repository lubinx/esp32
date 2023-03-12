#ifndef __SYS_PTHREADTYPES_H
#define	__SYS_PTHREADTYPES_H            1

#include <sys/sched.h>

/***************************************************************************
 *  @def: pthread_key_t
 ***************************************************************************/
    typedef void *(*pthread_routine_t)(void *arg);
    typedef __uint32_t pthread_t;            /* identify a thread */

    struct pthread_attr_t
    {
        int stack_size;
        void *stack;

        int  detachstate;
    };
    typedef struct pthread_attr_t   pthread_attr_t;

/***************************************************************************
 *  @def: pthread_key_t
 ***************************************************************************/
    typedef __uint32_t pthread_key_t;        /* thread-specific data keys */

/***************************************************************************
 *  @def: pthread_condattr_t
 ***************************************************************************/
    typedef __uint32_t pthread_cond_t;       /* identify a condition variable */

    typedef struct pthread_condattr_t
    {
        int dummy;
    };
    typedef struct pthread_condattr_t   pthread_condattr_t;

/***************************************************************************
 *  @def: pthread_once_t
 ***************************************************************************/
    struct pthread_once_t
    {
        int dummy;
    };
    typedef struct pthread_once_t   pthread_once_t;

/***************************************************************************
 *  @def: pthread_spinlock_t
 ***************************************************************************/
// pthread spinlock types
    struct pthread_spinlock_t
    {
        pthread_t volatile owner;
    };
    typedef struct pthread_spinlock_t   pthread_spinlock_t;

    #define PTHREAD_SPINLOCK_INITIALIZER   {0}

/***************************************************************************
 *  @def: pthread_mutex_t
 ***************************************************************************/
    /**
     *  pmutex_mutex_t *MUST* define as a ptr
     *      somehow libstdc++.a(eh_alloc.o) was a created a mutex, by using uintptr_t version of pthread_mutex_t, *stupid*
     *      the correct way is using sys/lock.h, use a static initialized lock...or just pick one that already there
    */
    typedef uintptr_t               pthread_mutex_t;

    struct pthread_mutexattr_t
    {
        int type;
    };
    typedef struct pthread_mutexattr_t  pthread_mutexattr_t;

    #define PTHREAD_MUTEX_INITIALIZER   \
        ((uintptr_t)-1)
    #define PTHREAD_RECURSIVE_MUTEX_INITIALIZER \
        ((uintptr_t)-2)

/***************************************************************************
 *  @def: pthread_rwlockattr_t
 ***************************************************************************/
    typedef struct pthread_rwlockattr_t pthread_rwlockattr_t;
    typedef struct pthread_rwlock_t pthread_rwlock_t;

#endif
