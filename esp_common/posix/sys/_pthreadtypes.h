#ifndef __SYS_PTHREADTYPES_H
#define	__SYS_PTHREADTYPES_H            1

#include <sys/sched.h>

    typedef void *(*pthread_routine_t)(void *arg);
    typedef __uint32_t pthread_t;            /* identify a thread */

    typedef struct
    {
        int is_initialized;
        void *stackaddr;
        int stacksize;
        int contentionscope;
        int inheritsched;
        int schedpolicy;
        struct sched_param schedparam;

        /* P1003.4b/D8, p. 54 adds cputime_clock_allowed attribute.  */
        #if defined(_POSIX_THREAD_CPUTIME)
        int  cputime_clock_allowed;  /* see time.h */
        #endif
        int  detachstate;
    } pthread_attr_t;

    /* Condition Variables */

    typedef __uint32_t pthread_cond_t;       /* identify a condition variable */

    #define _PTHREAD_COND_INITIALIZER ((pthread_cond_t) 0xFFFFFFFF)

    typedef struct
    {
        int      is_initialized;
        clock_t  clock;             /* specifiy clock for timeouts */
        // int      process_shared;    /* allow this to be shared amongst processes */
    } pthread_condattr_t;         /* a condition attribute object */

    /* Keys */

    typedef __uint32_t pthread_key_t;        /* thread-specific data keys */

    typedef struct
    {
        int   is_initialized;  /* is this structure initialized? */
        int   init_executed;   /* has the initialization routine been run? */
    } pthread_once_t;       /* dynamic package initialization */

    #define _PTHREAD_ONCE_INIT  { 1, 0 }  /* is initialized and not run */

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
