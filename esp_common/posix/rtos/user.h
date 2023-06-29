/* NOTE: this is not full porting ultracore for esp32
    not all function is implemented
*/

/****************************************************************************
  This file is part of UltraCore

  Copyright by UltraCreation Co Ltd 2018
-------------------------------------------------------------------------------
    The contents of this file are used with permission, subject to the Mozilla
  Public License Version 1.1 (the "License"); you may not use this file except
  in compliance with the License. You may  obtain a copy of the License at
  http://www.mozilla.org/MPL/MPL-1.1.html

    Software distributed under the License is distributed on an "AS IS" basis,
  WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License for
  the specific language governing rights and limitations under the License.
****************************************************************************/
#ifndef __RTOS_USER_H
#define __RTOS_USER_H                   1

#include <features.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/errno.h>
#include <rtos/types.h>

__BEGIN_DECLS

/***************************************************************************/
/** generic waitfor
****************************************************************************/
    #define WAIT_FOREVER                (0xFFFFFFFFUL)
    #define SLEEP_FOREVER               WAIT_FOREVER

    /**
     *  waitfor() generic waitfor synchronize objects
     *      @param hdr
     *          Semaphore/Event/Mutex and Thread
     *      @param timeout
     *          wait timeout in milliseconds
     *      @returns
     *          On Success 0 is returned
     *          On Error an errno shall be returned to indicate the error
     *      @errors
     *          EINVAL: indicate hdr is not valid syncobj
     *          ETIMEDOUT: syncobj can not acquire by timeout
     *          EACCES: synobj is thread/mutex/rwlock, and it acquired from ISR
     *          EDEADLK: synobj is mutex/rwlock
     */
extern __attribute__((nothrow))
    int waitfor(handle_t hdl, uint32_t timeout);

/***************************************************************************/
/** thread
****************************************************************************/
    #define THREAD_DEFAULT_STACK_SIZE           (4096)
    #define THREAD_MINIMAL_STACK_SIZE           (1024)  // see FreeRTOSConfig.h => configMINIMAL_STACK_SIZE

    #define THREAD_DEFAULT_PRIORITY             (3)     // see FreeRTOSConfig.h => configMAX_PRIORITIES
    #define THREAD_NO_CORE_AFFINITY             ((unsigned)~0)

    /**
     *  thread_create()
     *      by using sdkconfig default priority, and THREAD_NO_CORE_AFFINITY
    */
extern __attribute__((nothrow, nonnull(2)))
    thread_id_t thread_create(void *(*start_rountine)(void *arg), void *arg, uint8_t priority,
        uint32_t *stack, size_t stack_size);

    /**
     *  thread_create_at_core()
     *      @returns
     *          On Success 0 is returned
     *          On error, -1 is returned, and errno is set to indicate the error
     *      @errors
     *          EINVAL: stack_size < CONFIG_PTHREAD_STACK_MIN or core_id > SOC_CPU_CORES_NUM
     *          ENOMEM
    */
extern __attribute__((nothrow, nonnull(2)))
    thread_id_t thread_create_at_core(void *(*start_rountine)(void *arg), void *arg, uint8_t priority,
        uint32_t *stack, size_t stack_size, unsigned affinity);

extern __attribute__((nothrow))
    thread_id_t thread_self(void);

extern __attribute__((nothrow, nonnull))
    int thread_join(thread_id_t thread);

extern __attribute__((nothrow, nonnull))
    int thread_detach(thread_id_t thread);

/***************************************************************************/
/** @mqueue
****************************************************************************/
    #define MQUEUE_MAX_MSG_SIZE         (1024)
    #define MQUEUE_MAX_MSG              (128)

    /**
     *  mqueue_create(): create a mqueue
     *      @returns
     *          On Success hdr to mqueue is returned
     *          On error, -1 is returned, and errno is set to indicate the error
     *      @errors
     *          ENOMEM
     *          EEXIST
     */
extern __attribute__((nothrow))
    int mqueue_create(char const *name, uint16_t msg_size, uint16_t msg_count);

    /**
     *  mqueue_destroy()
     *      @returns
     *          On Success 0 is returned
     *          On Error an errno shall be returned to indicate the error
     *      @errors
     *          EBADF
     */
extern __attribute__((nothrow))
    int mqueue_destroy(int mqd);

    /**
     *  mqueue_queued()
     */
extern __attribute__((nothrow))
    int mqueue_queued(int mqd);

    /**
     *  mqueue_flush(): flush all messages from mqueue
     *      @returns
     *          On Success 0 is returned
     *          On Error an errno shall be returned to indicate the error
     *      @errors
     *          EBADF
     */
extern __attribute__((nothrow))
    int mqueue_flush(int mqd);

    /**
     *  mqueue_recv() / mqueue_timedrecv()
     *      detail see unistd.h read()
     */
extern __attribute__((nonnull(2), nothrow))
    ssize_t mqueue_recv(int mqd, void *restrict msg, unsigned int *restrict prio);
extern __attribute__((nonnull(2), nothrow))
    ssize_t mqueue_timedrecv(int mqd, void *restrict msg, uint32_t timeout, unsigned int *restrict prio);

    /**
     *  mqueue_send() / mqueue_timedsend()
     *      detail see unistd.h write()
     */
extern __attribute__((nonnull, nothrow))
    ssize_t mqueue_send(int mqd, void const *msg, unsigned int prio);
extern __attribute__((nonnull, nothrow))
    ssize_t mqueue_timedsend(int mqd, void const *msg, uint32_t timeout, unsigned int prio);

__END_DECLS
#endif
