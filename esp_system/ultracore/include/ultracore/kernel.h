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

#include "glist.h"
#include "atomic.h"
// #include "ultracore.h"
// #include "svc.h"

    #define INVALID_HANDLE              ((void *)0)
    typedef void *                  handle_t;

    #define CID_FREED                   (0x00)
    #define CID_TIMEOUT                 (0x01)
    #define CID_TCB                     (0x02)
    #define CID_FD                      (0x03)
    // synchronize objects, also defined in types.h
    #define CID_SYNCOBJS                (0x80)
    #define CID_SEMAPHORE               (CID_SYNCOBJS + 0)
    #define CID_EVENT                   (CID_SYNCOBJS + 1)
    #define CID_MUTEX                   (CID_SYNCOBJS + 2)
    #define CID_RWLOCK                  (CID_SYNCOBJS + 3)

    struct KERNEL_hdl
    {
        void        *glist_next;
        /// +0x04
        uint8_t     cid;                // default CID_FREE
        uint8_t     flags;
        uint8_t     rsv[sizeof(uintptr_t) - 2];
        /// ...padding to 64 bytes
        uintptr_t   padding[64 - 8];
    };
    #define AsKernelHdl(handle)           ((struct KERNEL_hdl *)handle)

    struct FD_implement
    {
        ssize_t (* read)  (int fd, void *buf, size_t bufsize);
        ssize_t (* write) (int fd, void const *buf, size_t count);
        off_t   (* seek)  (int fd, off_t offset, int origin);
        int     (* close) (int fd);
        int     (* ioctl) (int fd, unsigned long int request, va_list vl);
    };

/***************************************************************************/
/** @flags
****************************************************************************/
    /// indicate the handle was destroying
    #define HDL_FLAG_DESTROYING         (1U << 7)
    /// indicate the handle memory was managed by system
    #define HDL_FLAG_SYSMEM             (1U << 6)

    /// flags 0~5 is preserved for object use

/***************************************************************************/
/** @file descriptor
****************************************************************************/
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

    struct KERNEL_fd
    {
        void        *glist_next;
        uint8_t     cid;
        uint8_t     flags;
        uint16_t    tag;
    ///----
        /// fd'io implement
        struct FD_implement const *implement;
        /// ext parameters
        ///     struct FS_ext * when fs is not NULL
        ///     otherwise pointer to fd's customize data
        void *ext;
        /// more detail see @filesystem.h
        ///     fs will contain a none NULL struct FS_implement when fd is opened by filesystem
        //      and ext will point to a valid struct FS_ext
        struct FS_implement const *fs;
        // + 4
        handle_t read_rdy;
        handle_t write_rdy;

        /// +6 changing by seek
        uintptr_t position;
        uint32_t read_timeo, write_timeo;
    };
    #define AsFD(handle)                ((struct KERNEL_fd *)handle)

/***************************************************************************/
/** @virtual file descriptor
****************************************************************************/
    struct KERNEL_vfd
    {
        void        *glist_next;
        uint8_t     cid;
        uint8_t     flags;
        uint16_t    tag;
    ///---- fd
        struct FD_implement const *implement;
        void *ext;
        struct FS_implement const *fs;
        handle_t read_rdy;
        handle_t write_rdy;
        uintptr_t position;
    ///----
        uint32_t size;                  // read_timeo => size
    };
    #define AsVFD(handle)               ((struct CORE_vfd *)handle)

/***************************************************************************/
/** @mqueue file descriptor
****************************************************************************/
    struct KERNEL_mqd
    {
        void        *glist_next;
        uint8_t     cid;
        uint8_t     flags;
        uint16_t    tag;
    ///---- fd
        struct FD_implement const *implement;
        void *ext;

        char const *name;               // fs => name: posix compatiable
        struct MQ_list *queued;         // read_rdy => queued
        struct MQ_list *freed;          // write_rdy => freed
        uint16_t msg_size;              // position => msg_size & msg_max
        uint16_t msg_max;
    ///----
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
    handle_t KERNEL_handle_get(uint32_t id);

    /**
     *  KERNEL_handle_release(): destroy a handle
     *      @returns
     *          On Success 0 is returned
     *          On Error an error number shall be returned to indicate the error
     *      @errors
     *          EBADF
     *          EFAULT: when handle is static initialized
     */
extern __attribute__((nonnull, nothrow))
    int KERNEL_handle_release(handle_t handle);

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
