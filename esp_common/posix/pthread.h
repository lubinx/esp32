/****************************************************************************
  This file is part of UltraCore
  Copyright by UltraCreation Co Ltd 2018

  Reference from IEEE Standard for Information Technology -
    Portable Operating System Interface (POSIX®) Issue 7 2018 Edition
  The original  Standard can be obtained online at
    http://www.unix.org/online.html.
-------------------------------------------------------------------------------
    The contents of this file are used with permission, subject to the Mozilla
  Public License Version 1.1 (the "License"); you may not use this file except
  in compliance with the License. You may  obtain a copy of the License at
  http://www.mozilla.org/MPL/MPL-1.1.html

    Software distributed under the License is distributed on an "AS IS" basis,
  WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License for
  the specific language governing rights and limitations under the License.
****************************************************************************/
#ifndef __PTHREAD_H
#define __PTHREAD_H                     1

#include <features.h>
#include <sys/types.h>

#ifndef __GCC__
    #ifndef __KEIL_CC__
    #pragma GCC system_header
    #endif
#endif
#ifdef __clang__
    #pragma clang system_header
#endif

// pthread spinlock types
    struct pthread_spinlock_t
    {
        pthread_t volatile owner;
    };
    typedef struct pthread_spinlock_t   pthread_spinlock_t;

    #define PTHREAD_SPINLOCK_INITIALIZER   {0}

// TODO: porting
#include <sys/_pthreadtypes.h>
    typedef void *(*pthread_routine_t)(void *arg);

    typedef struct pthread_rwlockattr_t pthread_rwlockattr_t;
    typedef struct pthread_rwlock_t pthread_rwlock_t;

    #define PTHREAD_MUTEX_INITIALIZER   _PTHREAD_MUTEX_INITIALIZER

/*
// Detach state.
    enum
    {
        PTHREAD_CREATE_JOINABLE,
        PTHREAD_CREATE_DETACHED
    };

// Mutex types.
    enum
    {
        PTHREAD_MUTEX_NORMAL,
        PTHREAD_MUTEX_RECURSIVE,
        PTHREAD_MUTEX_ERRORCHECK,
        PTHREAD_MUTEX_DEFAULT = PTHREAD_MUTEX_NORMAL
    };

// Mutex protocols.
    enum
    {
        PTHREAD_PRIO_NONE,
        PTHREAD_PRIO_INHERIT,
        PTHREAD_PRIO_PROTECT
    };

// Scheduler inheritance.
    enum
    {
        PTHREAD_INHERIT_SCHED,
        PTHREAD_EXPLICIT_SCHED
    };

// Scope handling.
    enum
    {
        PTHREAD_SCOPE_PROCESS,
        PTHREAD_SCOPE_SYSTEM
    };
*/

// Process shared or private flag.
    enum
    {
        PTHREAD_PROCESS_PRIVATE,
        PTHREAD_PROCESS_SHARED
    };

// Conditional variable handling.
    enum
    {
        PTHREAD_CANCEL_ENABLE,
        PTHREAD_CANCEL_DISABLE
    };
    enum
    {
        PTHREAD_CANCEL_DEFERRED,
        PTHREAD_CANCEL_ASYNCHRONOUS
    };
    #define PTHREAD_CANCELED            ((void *) -1)

// Single execution handling.
    #define PTHREAD_ONCE_INIT           0

__BEGIN_DECLS

    /**
     *  pthread_atfork()
     *      register fork handlers
     *      **NO SUPPORT** always returns ENOSYS
     */
    /*
extern __attribute__((nonnull, nothrow))
    int pthread_atfork(pthread_routine_t prepare, pthread_routine_t parent, pthread_routine_t child);
    */

//--------------------------------------------------------------------------
//  pthread attr
//--------------------------------------------------------------------------
    /**
     *  pthread_attr_init() & pthread_attr_destroy()
     *      initialize & destroy the thread attributes object
     */
extern __attribute__((nonnull, nothrow))
    int pthread_attr_init(pthread_attr_t *attr);
extern __attribute__((nonnull, nothrow))
    int pthread_attr_destroy(pthread_attr_t *attr);

    /**
     *  pthread_attr_getdetachstate() & pthread_attr_setdetachstate()
     *      get and set the detachstate attribute
     *  **ALWAYS** PTHREAD_CREATE_JOINABLE
     */
extern __attribute__((nonnull, nothrow))
    int pthread_attr_getdetachstate(pthread_attr_t const *restrict attr, int *detachstate);
extern __attribute__((nonnull, nothrow))
    int pthread_attr_setdetachstate(pthread_attr_t *restrict attr, int detachstate);

    /**
     *  pthread_attr_getguardsize() or pthread_attr_setguardsize()
     *      get and set the thread guardsize attribute
     *  **NO SUPPORT** always returns ENOSYS
     */
extern __attribute__((nonnull, nothrow))
    int pthread_attr_getguardsize(pthread_attr_t const *restrict attr, size_t *restrict guardsize);
extern __attribute__((nonnull, nothrow))
    int pthread_attr_setguardsize(pthread_attr_t *attr, size_t guardsize);

    /**
     *  pthread_attr_getinheritsched(), pthread_attr_setinheritsched()
     *      get and set the inheritsched attribute (REALTIME THREADS)
     *  **ALWAYS** PTHREAD_INHERIT_SCHED
     */
extern __attribute__((nonnull, nothrow))
    int pthread_attr_getinheritsched(pthread_attr_t const *restrict attr, int *restrict inheritsched);
extern __attribute__((nonnull, nothrow))
    int pthread_attr_setinheritsched(pthread_attr_t *attr, int inheritsched);

    /**
     *  pthread_attr_getschedparam(), pthread_attr_setschedparam()
     *      get and set the schedparam attribute
     *  **NO SUPPORT** always returns ENOSYS
     */
extern __attribute__((nonnull, nothrow))
    int pthread_attr_getschedparam(pthread_attr_t const *restrict attr, struct sched_param *restrict param);
extern __attribute__((nonnull, nothrow))
    int pthread_attr_setschedparam(pthread_attr_t *restrict attr, struct sched_param const *restrict param);

    /**
     *  pthread_attr_getschedpolicy(), pthread_attr_setschedpolicy()
     *      get and set the schedpolicy attribute (REALTIME THREADS)
     *  **ALWAYS** SCHED_FIFO
     */
extern __attribute__((nonnull, nothrow))
    int pthread_attr_getschedpolicy(pthread_attr_t const *restrict attr, int *policy);
extern __attribute__((nonnull, nothrow))
    int pthread_attr_setschedpolicy(pthread_attr_t * attr, int policy);

    /**
     *  pthread_attr_getscope(), pthread_attr_setscope()
     *      get and set the contentionscope attribute (REALTIME THREADS)
     *  **NO SUPPORT** pthread_attr_setscope() always returns ENOSYS
     */
extern __attribute__((nonnull, nothrow))
    int pthread_attr_getscope(pthread_attr_t const *restrict attr, int *scope);
extern __attribute__((nonnull, nothrow))
    int pthread_attr_setscope(pthread_attr_t *attr, int scope);

    /**
     *  pthread_attr_getstack(), pthread_attr_setstack()
     *      get and set stack attributes
     */
extern __attribute__((nonnull, nothrow))
    int pthread_attr_getstack(pthread_attr_t const *restrict attr, void **restrict stackaddr, size_t *restrict stacksize);
extern __attribute__((nonnull, nothrow))
    int pthread_attr_setstack(pthread_attr_t *attr, void *stackaddr, size_t stacksize);

    /**
     *  pthread_attr_getstacksize(), pthread_attr_setstacksize()
     *      get and set the stacksize attribute
     */
extern __attribute__((nonnull, nothrow))
    int pthread_attr_getstacksize(pthread_attr_t const *restrict attr, size_t *stacksize);
extern __attribute__((nonnull, nothrow))
    int pthread_attr_setstacksize(pthread_attr_t *attr, size_t stacksize);

//--------------------------------------------------------------------------
//  pthread misc
//--------------------------------------------------------------------------
    /**
     *  pthread_create(): thread creation
     */
extern __attribute__((nonnull(1, 3), nothrow))
    int pthread_create(pthread_t *restrict thread, pthread_attr_t const *restrict attr,
        pthread_routine_t routine, void *restrict arg);

    /**
     *  pthread_once(): dynamic package initialization
     *      toto: pthread_once()
     */
extern __attribute__((nonnull, nothrow))
    int pthread_once(pthread_once_t *thread, void (*)(void));

    /**
     *  pthread_detach(): detach a thread
     *  **NO SUPPORT** always returns ENOSYS
     */
extern __attribute__((nothrow))
    int pthread_detach(pthread_t id);

    /**
     *  pthread_self(): obtain ID of the calling thread
     */
extern __attribute__((nothrow))
    pthread_t pthread_self(void);

    /**
     *  pthread_join(): join with a terminated thread
     */
extern __attribute__((nothrow))
    int pthread_join(pthread_t thread, void **retval);

    /**
     *  pthread_cancel(): send a cancellation request to a thread
     *  **NO SUPPORT** always returns ENOSYS
     */
extern __attribute__((nothrow))
    int pthread_cancel(pthread_t thread);

    /**
     *  pthread_testcancel(): request delivery of any pending cancellation request
     */
extern __attribute__((nothrow))
    void pthread_testcancel(void);

    /**
     *  pthread_setcancelstate(), pthread_setcanceltype()
     *      set  cancelability state and type
     *  **NO SUPPORT** always returns ENOSYS
     */
extern __attribute__((nothrow))
    int pthread_setcancelstate(int state, int *oldstate);
extern __attribute__((nothrow))
    int pthread_setcanceltype(int type, int *old_type);

    /**
     *  pthread_yield()
     *      yield current thread
     *  **no standard** any more, @see sched_yield()
     */
extern __attribute__((nothrow))
    int pthread_yield(void);

    /**
     *  pthread_equal(): compare thread IDs
     */
extern __attribute__((nothrow))
    int pthread_equal(pthread_t t1, pthread_t t2);

    /**
     *  pthread_cleanup_push(), pthread_cleanup_pop()
     *      establish cancellation handlers
     */
extern __attribute__((nonnull(1), nothrow))
    void pthread_cleanup_push(void (*routine)(void*), void *arg);
extern __attribute__((nothrow))
    void pthread_cleanup_pop(int execute);

    /**
     *  pthread_exit(): terminate calling thread
     */
extern __attribute__((nothrow))
    void pthread_exit(void *);

    /**
     *  pthread_setconcurrency(), pthread_getconcurrency()
     *      set/get the concurrency level
     *  **ALWAYS** 0
     */
extern __attribute__((nothrow))
    int pthread_getconcurrency(void);
extern __attribute__((nothrow))
    int pthread_setconcurrency(int level);

//--------------------------------------------------------------------------
//  pthread key
//--------------------------------------------------------------------------
    /**
     *  pthread_key_create(): thread-specific data key creation
     */
extern __attribute__((nonnull(1), nothrow))
    int pthread_key_create(pthread_key_t *key, void (*destructor)(void *));
    /**
     *  pthread_key_delete(): thread-specific data key deletion
     */
extern __attribute__((nothrow))
    int pthread_key_delete(pthread_key_t key);

    /**
     *  pthread_getspecific(), pthread_setspecific()
     *      thread-specific data management
     */
extern __attribute__((nothrow))
    void *pthread_getspecific(pthread_key_t key);
extern __attribute__((nonnull(2), nothrow))
    int pthread_setspecific(pthread_key_t key, void const *val);

//--------------------------------------------------------------------------
//  pthread spinlock
//--------------------------------------------------------------------------
    /**
     *  pthread_spin_init() pthread_spin_destroy()
     */
extern __attribute__((nonnull, nothrow))
    int pthread_spin_init(pthread_spinlock_t *lock, int pshared);
extern __attribute__((nonnull, nothrow))
    int pthread_spin_destroy(pthread_spinlock_t *lock);

    /**
     *  pthread_spin_lock(), pthread_spin_trylock(), pthread_spin_unlock()
     */
extern __attribute__((nonnull, nothrow))
       int pthread_spin_lock(pthread_spinlock_t *lock);
extern __attribute__((nonnull, nothrow))
       int pthread_spin_trylock(pthread_spinlock_t *lock);
extern __attribute__((nonnull, nothrow))
       int pthread_spin_unlock(pthread_spinlock_t *lock);

//--------------------------------------------------------------------------
//  pthread mutex
//--------------------------------------------------------------------------
    /**
     *  pthread_mutexattr_init() pthread_mutexattr_destroy()
     *      destroy and initialize the mutex attributes object
     */
extern __attribute__((nonnull, nothrow))
    int pthread_mutexattr_init(pthread_mutexattr_t *attr);
extern __attribute__((nonnull, nothrow))
    int pthread_mutexattr_destroy(pthread_mutexattr_t *attr);

    /**
     *  pthread_mutexattr_getprioceiling(), pthread_mutexattr_setprioceiling()
     *      get and set the prioceiling attribute of the mutex attributes object (REALTIME THREADS)
     *  **NO SUPPORT** always returns 0
     */
extern __attribute__((nonnull, nothrow))
    int pthread_mutexattr_getprioceiling(const pthread_mutexattr_t *restrict attr, int *restrict prioceiling);
extern __attribute__((nonnull, nothrow))
    int pthread_mutexattr_setprioceiling(pthread_mutexattr_t *attr, int prioceiling);

    /**
     *  pthread_mutexattr_getprotocol(), pthread_mutexattr_setprotocol()
     *      get and set the protocol attribute of the mutex attributes object (REALTIME THREADS)
     *  **ALWAYS** PTHREAD_PRIO_NONE
     */
extern __attribute__((nonnull, nothrow))
    int pthread_mutexattr_getprotocol(const pthread_mutexattr_t *restrict attr, int *restrict protocol);
extern __attribute__((nonnull, nothrow))
    int pthread_mutexattr_setprotocol(pthread_mutexattr_t *attr, int protocol);

    /**
     *  pthread_mutexattr_getpshared(), pthread_mutexattr_setpshared()
     *      get and set the process-shared attribute
     *  **ALWAYS** PTHREAD_PROCESS_PRIVATE, since we are single process system
     */
extern __attribute__((nonnull, nothrow))
    int pthread_mutexattr_getpshared(const pthread_mutexattr_t *restrict attr, int *restrict pshared);
extern __attribute__((nonnull, nothrow))
    int pthread_mutexattr_setpshared(pthread_mutexattr_t *attr, int pshared);

    /**
     *  pthread_mutexattr_gettype(), pthread_mutexattr_settype()
     *      get and set the mutex type attribute
     */
extern __attribute__((nonnull, nothrow))
    int pthread_mutexattr_gettype(const pthread_mutexattr_t *restrict attr, int *restrict type);
extern __attribute__((nonnull, nothrow))
    int pthread_mutexattr_settype(pthread_mutexattr_t *attr, int type);

    /**
     *  pthread_mutex_init(), pthread_mutex_destroy()
     *      initialise or destroy a mutex
     */
extern __attribute__((nothrow))
    int pthread_mutex_init(pthread_mutex_t *restrict mutex, const pthread_mutexattr_t *restrict attr);
extern __attribute__((nonnull, nothrow))
    int pthread_mutex_destroy(pthread_mutex_t *mutex);

    /**
     *  pthread_mutex_getprioceiling(), pthread_mutex_setprioceiling()
     *      get and set the priority ceiling of a mutex
     *  **NO SUPPORT** always returns 0
     */
extern __attribute__((nonnull, nothrow))
    int pthread_mutex_getprioceiling(const pthread_mutex_t *restrict mutex, int *restrict prioceiling);
extern __attribute__((nonnull, nothrow))
    int pthread_mutex_setprioceiling(pthread_mutex_t *restrict mutex, int prioceiling, int *old_prioceiling);

    /**
     *  pthread_mutex_lock(), pthread_mutex_trylock(), pthread_mutex_unlock()
     *      lock and unlock a mutex
     */
extern __attribute__((nonnull, nothrow))
    int pthread_mutex_lock(pthread_mutex_t *mutex);
extern __attribute__((nonnull, nothrow))
    int pthread_mutex_trylock(pthread_mutex_t *mutex);
extern __attribute__((nonnull, nothrow))
    int pthread_mutex_unlock(pthread_mutex_t *mutex);

    /**
     *  pthread_mutex_consistent(): make a robust mutex consistent
     */
extern __attribute__((nonnull, nothrow))
    int pthread_mutex_consistent(pthread_mutex_t *mutex);

//--------------------------------------------------------------------------
//  pthread condition
//      https://en.wikipedia.org/wiki/Monitor_(synchronization)
//--------------------------------------------------------------------------
    /**
     *  pthread_condattr_destroy()/pthread_condattr_init()
     *      destroy and initialize the condition variable attributes object
     */
extern __attribute__((nonnull, nothrow))
    int pthread_condattr_init(pthread_condattr_t *cond);
extern __attribute__((nonnull, nothrow))
    int pthread_condattr_destroy(pthread_condattr_t *cond);

    /**
     *  pthread_condattr_getpshared()/pthread_condattr_setpshared()
     *      get and set the process-shared condition variable attributes
     *  **ALWAYS** PTHREAD_PROCESS_PRIVATE
     */
extern __attribute__((nonnull, nothrow))
    int pthread_condattr_getpshared(const pthread_condattr_t *restrict attr, int *restrict pshared);
extern __attribute__((nonnull, nothrow))
    int pthread_condattr_setpshared(pthread_condattr_t *attr, int pshared);

    /**
     *  pthread_cond_init() / pthread_cond_destroy():
     *      initialize destroy and condition variables
     */
extern __attribute__((nonnull, nothrow))
    int pthread_cond_init(pthread_cond_t *restrict cond, const pthread_condattr_t *restrict attr);
extern __attribute__((nonnull, nothrow))
    int pthread_cond_destroy(pthread_cond_t *cond);

    /**
     *  pthread_cond_broadcast()/pthread_cond_signal()
     *      broadcast or signal a condition
     */
extern __attribute__((nonnull, nothrow))
    int pthread_cond_broadcast(pthread_cond_t *cond);
extern __attribute__((nonnull, nothrow))
    int pthread_cond_signal(pthread_cond_t *cond);

extern __attribute__((nonnull, nothrow))
    int pthread_cond_timedwait(pthread_cond_t *restrict cond, pthread_mutex_t *restrict mutex,
        struct timespec const *restrict ts);
extern __attribute__((nonnull, nothrow))
    int pthread_cond_wait(pthread_cond_t *restrict cond, pthread_mutex_t *restrict mutex);

//--------------------------------------------------------------------------
//  pthread rwlock
//      using two mutexes to simuliate read-preferring rwlock
//      https://en.wikipedia.org/wiki/Readers%E2%80%93writer_lock
//--------------------------------------------------------------------------
    /**
     *  pthread_rwlockattr_init() / pthread_rwlockattr_destroy()
     *      initialize and destroy the read-write lock attributes object
     */
extern __attribute__((nonnull, nothrow))
    int pthread_rwlockattr_init(pthread_rwlockattr_t *attr);
extern __attribute__((nonnull, nothrow))
    int pthread_rwlockattr_destroy(pthread_rwlockattr_t *attr);

    /**
     *  pthread_rwlockattr_getpshared(), pthread_rwlockattr_setpshared()
     *      get and set the process-shared attribute
     *  **ALWAYS** PTHREAD_PROCESS_PRIVATE, since we are single process system
     */
extern __attribute__((nonnull, nothrow))
    int pthread_rwlockattr_getpshared(pthread_rwlockattr_t const *restrict attr, int *restrict pshared);
extern __attribute__((nonnull, nothrow))
    int pthread_rwlockattr_setpshared(pthread_rwlockattr_t *attr, int shared);

    /**
     *  pthread_rwlock_init() / thread_rwlock_destroy():
     *      initialize and destroy a read-write lock object
     *  **recursion mutex**
     */
extern __attribute__((nothrow))
    int pthread_rwlock_init(pthread_rwlock_t *restrict rwlock, pthread_rwlockattr_t const *restrict attr);
extern __attribute__((nonnull, nothrow))
    int pthread_rwlock_destroy(pthread_rwlock_t *rwlock);

extern __attribute__((nonnull, nothrow))
    int pthread_rwlock_rdlock(pthread_rwlock_t *rwlock);
extern __attribute__((nonnull, nothrow))
    int pthread_rwlock_tryrdlock(pthread_rwlock_t *rwlock);

extern __attribute__((nonnull, nothrow))
    int pthread_rwlock_wrlock(pthread_rwlock_t *rwlock);
extern __attribute__((nonnull, nothrow))
    int pthread_rwlock_trywrlock(pthread_rwlock_t *rwlock);

extern __attribute__((nonnull, nothrow))
    int pthread_rwlock_unlock(pthread_rwlock_t *rwlock);

__END_DECLS
#endif
