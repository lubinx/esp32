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
#include <sys/types.h>
#include <rtos/types.h>

__BEGIN_DECLS
/***************************************************************************/
/** generic waitfor
****************************************************************************/
extern __attribute__((nothrow, nonnull(2)))
    thread_id_t thread_create(unsigned priority, void *(*start_rountine)(void *arg), void *arg, uint32_t *stack, size_t stack_size);

extern __attribute__((nothrow))
    thread_id_t thread_self(void);

extern __attribute__((nothrow, nonnull))
    int thread_join(thread_id_t thread);

extern __attribute__((nothrow, nonnull))
    int thread_detach(thread_id_t thread);

/***************************************************************************/
/** generic waitfor
****************************************************************************/
    // infinite timeout
    #define INFINITE                    (0xFFFFFFFFUL)

    /**
     *  waitfor() generic waitfor synchronize objects
     *      @param hdr
     *          Semaphore/Event/Mutex and Thread
     *      @param timeout
     *          wait timeout in milliseconds
     *      @returns
     *          On Success 0 is returned
     *          On Error an error number shall be returned to indicate the error
     *      @errors
     *          EINVAL: indicate hdr is not valid syncobj
     *          ETIMEDOUT: syncobj can not acquire by timeout
     *          EACCES: synobj is thread/mutex/rwlock, and it acquired from ISR
     *          EDEADLK: synobj is mutex/rwlock
     */
extern __attribute__((nothrow))
    int waitfor(handle_t hdl, uint32_t timeout);

__END_DECLS
#endif
