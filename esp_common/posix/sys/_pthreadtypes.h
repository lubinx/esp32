#ifndef __SYS_PTHREADTYPES_H
#define	__SYS_PTHREADTYPES_H            1

#include <sys/sched.h>

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

#endif
