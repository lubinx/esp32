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
#ifndef __KERNEL_KERNEL_H
#define __KERNEL_KERNEL_H                 1

#include <features.h>

#include <stdint.h>
#include <stdarg.h>
#include <sys/types.h>
#include <sys/mutex.h>

#include <rtos/glist.h>
#include <rtos/types.h>
#include <rtos/user.h>
#include <rtos/filesystem.h>

#include <xtensa/spinlock.h>

/***************************************************************************/
/** @TCB
****************************************************************************/
    struct KERNEL_tcb
    {
        void        *glist_next;
        uint8_t     cid;
        uint8_t     flags;
        uint8_t     rsv[sizeof(uintptr_t) - 2];

        void *(*start_routine)(void *arg);
        void *arg;
        void *exit_code;

        uint32_t    stack_size;
        void        *stack_base;
    };
    #define AsTCB(handle)               ((struct KERNEL_tcb *)handle)

/***************************************************************************/
/** @file descriptor
****************************************************************************/
    struct FD_implement
    {
        ssize_t (* read)  (int fd, void *buf, size_t bufsize);
        ssize_t (* write) (int fd, void const *buf, size_t count);
        off_t   (* seek)  (int fd, off_t offset, int origin);
        int     (* close) (int fd);
        int     (* ioctl) (int fd, unsigned long int request, va_list vl);
    };

    /// indicate the fd is non-block
    #define FD_FLAG_NONBLOCK            (1U << 0)

    // block speical
    #define FD_TAG_BLOCK                (1U << 0)
    // character special
    #define FD_TAG_CHAR                 (1U << 1)
    // directory
    #define FD_TAG_DIR                  (1U << 2)
    // pipe
    #define FD_TAG_FIFO                 (1U << 3)
    // regular file
    #define FD_TAG_REG                  (1U << 4)
    // symbolic link
    #define FD_TAG_LINK                 (1U << 5)
    // socket
    #define FD_TAG_SOCKET               (1U << 6)
    // virtual fd
    #define FD_TAG_VFD                  (1U << 7)
    // mqd
    #define FD_TAG_MQD                  (FD_TAG_VFD | FD_TAG_BLOCK | FD_TAG_FIFO)

    struct KERNEL_fd
    {
        void        *glist_next;
        uint8_t     cid;
        uint8_t     flags;
        uint16_t    tag;
    ///----
        // fd'io implement
        struct FD_implement const *implement;
        // filesystem
        //  fs and fdio is valid when fd is open by filesystem
        struct FS_implement const *fs;

        // fd parameters
        union // __attribute__((packed))
        {
            /// fs parameters
            void *fsio;
            /// fd ext parameters
            void *ext;
        };
        // fd syncobjs
        handle_t read_rdy;
        handle_t write_rdy;
        /// seek
        uintptr_t position;
        uint32_t read_timeo, write_timeo;
    };
    #define AsFD(handle)                ((struct KERNEL_fd *)handle)

/***************************************************************************/
/** @mqueue file descriptor
****************************************************************************/
    struct KERNEL_mqd
    {
        struct KERNEL_mqd *glist_next;
        uint8_t     cid;
        uint8_t     flags;
        uint16_t    tag;
    ///---- fd
        struct FD_implement const *implement;
        char const *name;               // NOTE: fs => name: posix compatiable
        void *ext;
        handle_t read_rdy;
        handle_t write_rdy;
    ///----
        void *rsv;                      // NOTE: fd'position is no used by mqd
        uint32_t read_timeo, write_timeo;
    };
    #define AsMqd(handle)               ((struct KERNEL_mqd *)handle)

__BEGIN_DECLS

/***************************************************************************/
/** kernel functions
****************************************************************************/
    /**
     *  KERNEL_handle_get(): create a handle
     *      @returns
     *          On Success handle pointer is returned
     *          On error, NULL is returned, and errno is set to indicate the error
     *      @errors
     *          ENOMEM
     */
extern __attribute__((nothrow))
    handle_t KERNEL_handle_get(uint8_t cid);

    /**
     *  KERNEL_handle_release(): destroy a handle
     *      @returns
     *          On Success 0 is returned
     *          On Error an errno shall be returned to indicate the error
     *      @errors
     *          EBADF
     *          EFAULT: when handle is static initialized
     */
extern __attribute__((nonnull, nothrow))
    int KERNEL_handle_release(handle_t handle);

    /**
     *  KERNEL_handle_recycle():
     *      NOTE: *MUST* call from rtos/idle to recycle released handle
     */
extern __attribute__((nonnull, nothrow))
    void KERNEL_handle_recycle(void);

    /**
     *   KERNEL_createfd(): create a file descriptor
     *      @returns
     *          On Success fd is returned
     *          On error, -1 is returned, and errno is set to indicate the error
     *      @errors
     *          ENOMEM
     */
extern __attribute__((nonnull(2), nothrow))
    int KERNEL_createfd(uint16_t const TAG, struct FD_implement const *implement, void *ext);

    /**
     *  KERNEL_malloc(): allocate memory
     */
extern __attribute__((nothrow, pure))
    void *KERNEL_malloc(uint32_t size);

    /**
     *  KERNEL_mallocz(): allocate memory and zero it
     */
extern __attribute__((nothrow))
    void *KERNEL_mallocz(uint32_t size);

    /**
     *  KERNEL_mfree()
     */
extern __attribute__((nonnull, nothrow))
    void KERNEL_mfree(void *ptr);

/***************************************************************************/
/** @kickless
****************************************************************************/
    /**
     *  KERNEL_add_ticks()
     *      increase millisecond to KERNEL_context.tick
     */
extern __attribute__((nothrow))
    void KERNEL_add_ticks(uint32_t millisecond);

    /**
     *  KERNEL_next_tick()
     *      implement tickless
     */
extern __attribute__((nothrow))
    void KERNEL_next_tick(uint32_t millisecond);

__END_DECLS
#endif
