#ifndef __GENERIC_OS_TYPES_H
#define __GENERIC_OS_TYPES_H            1

#include <features.h>
#include <stdint.h>

    #define INVALID_HANDLE              ((void *)0)
    typedef void *                  handle_t;

    struct KERNEL_hdl
    {
        void        *glist_next;
        uint8_t     cid;                // default CID_FREE
        uint8_t     flags;
        uint8_t     rsv[sizeof(uintptr_t) - 2];

        union
        {
            // initializer
            struct
            {
                uint32_t max_count;
                uint32_t initial_count;
            } init_sem;

            // StaticSemaphore_t padding
            uintptr_t   padding[20];
        };
    };
    #define AsKernelHdl(hdl)            ((struct KERNEL_hdl *)hdl)

// cid
    #define CID_FREED                   (0x00)
    #define CID_TIMEOUT                 (0x01)
    #define CID_TCB                     (0x02)
    #define CID_FD                      (0x03)
    // synchronize objects
    #define CID_SYNCOBJS                (0x80)
    #define CID_SEMAPHORE               (CID_SYNCOBJS + 0)
    #define CID_EVENT                   (CID_SYNCOBJS + 1)
    #define CID_MUTEX                   (CID_SYNCOBJS + 2)
    #define CID_RWLOCK                  (CID_SYNCOBJS + 3)

// flags
    /// indicate the handle was destroying
    #define HDL_FLAG_DESTROYING         (1U << 7)
    /// indicate the handle memory was managed by system
    #define HDL_FLAG_SYSMEM_MANAGED     (1U << 6)
    /// indicate freertos StaticSemaphore_t is NOT allow in intr, like mutex
    #define HDL_FLAG_NO_INTR            (1U << 5)
    /// indicate freertos StaticSemaphore_t is NOT initialized, by using INITIALIZER
    #define HDL_FLAG_INITIALIZER        (1U << 4)
    /// indicate freertos StaticSemaphore_t is recursive (mutex)
    #define HDL_FLAG_FREERTOS_RECURSIVE (1U << 3)

/***************************************************************************
 *  @def: mutex
 ***************************************************************************/
    typedef struct KERNEL_hdl       mutex_t;

    // mutex_create() flags
    #define MUTEX_FLAG_NORMAL           (0x00)
    #define MUTEX_FLAG_RECURSIVE        (HDL_FLAG_FREERTOS_RECURSIVE)

    // mutex initializer
    #define MUTEX_INITIALIZER           {.cid = CID_MUTEX, .flags = HDL_FLAG_INITIALIZER | MUTEX_FLAG_NORMAL}
    #define MUTEX_RECURSIVE_INITIALIZER {.cid = CID_MUTEX, .flags = HDL_FLAG_INITIALIZER | MUTEX_FLAG_RECURSIVE}

#endif
