/****************************************************************************
  This file is part of UltraCore

  Copyright by UltraCreation Co Ltd 2018
-------------------------------------------------------------------------------
    The contents of this file are used with permission, subject to the Mozilla
  Public License Version 1.1(the "License"); you may not use this file except
  in compliance with the License. You may  obtain a copy of the License at
  http://www.mozilla.org/MPL/MPL-1.1.html

    Software distributed under the License is distributed on an "AS IS" basis,
  WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License for
  the specific language governing rights and limitations under the License.
****************************************************************************/
#ifndef __SEMAPHORE_H
#define __SEMAPHORE_H                   1

#include <features.h>
#include <sys/_timespec.h>

    typedef struct KERNEL_hdl      sem_t;

    /* MAX semaphore supported (2^31) */
#ifndef SEM_VALUE_MAX
    #define SEM_VALUE_MAX               (0x7FFFFFFF)
#endif

    /* Value returned if `sem_open' failed.  */
#ifndef SEM_FAILED
    #define SEM_FAILED                  ((void *)0)
#endif

__BEGIN_DECLS

    /**
     * sem_init(): initialize an unnamed semaphore
     */
extern __attribute__((nonnull, nothrow))
    int sem_init(sem_t *sema, int pshared, unsigned int value);

    /**
     *  sem_destroy(): destroy an unnamed semaphore
     */
extern __attribute__((nonnull, nothrow))
    int sem_destroy(sem_t *sema);

    /**
     *  sem_open(): initialize and open a named semaphore
     */
extern __attribute__((nonnull, nothrow))
    sem_t *sem_open(char const *name, int oflag, ...);

    /**
     *  sem_close(): close a named semaphore
     */
extern __attribute__((nonnull, nothrow))
    int sem_close(sem_t *sema);

    /**
     *  sem_unlink(): remove a named semaphore
     */
extern __attribute__((nonnull, nothrow))
    int sem_unlink(char const *name);

    /**
     *  sem_wait()
     */
extern __attribute__((nonnull, nothrow))
    int sem_wait(sem_t *sema);
extern __attribute__((nonnull, nothrow))
    int sem_timedwait(sem_t *restrict sema, struct timespec const *restrict spec);

    /**
     *  sem_timedwait_ms()
     *      extend sem_timedwait() by using milli-second
     *  NOTE: not posix
    */
extern __attribute__((nonnull, nothrow))
    int sem_timedwait_ms(sem_t *sema, unsigned int millisecond);
static inline
    int sem_trywait(sem_t *sema) { return sem_timedwait_ms(sema, 0); }

    /**
     *  sem_post()
     */
extern __attribute__((nonnull, nothrow))
    int sem_post(sem_t *sema);

    /**
     *  sem_getvalue(): get the value of a semaphore
     */
extern __attribute__((nonnull, nothrow))
    int sem_getvalue(sem_t *restrict sema, int *restrict val);

__END_DECLS
#endif
