#include <string.h>
#include <sys/errno.h>
#include <sys/lock.h>

#include "ultracore/kernel.h"
#include "esp_log.h"

static char const *TAG = "kernel";
#define DYNAMIC_INC_DESCRIPTORS         (1024 / sizeof(sizeof(struct KERNEL_hdl)))

struct KERNEL_context_t
{
    struct KERNEL_hdl hdl[4096 / sizeof(struct KERNEL_hdl)];
    glist_t hdl_freed_list;
};
struct KERNEL_context_t KERNEL_context;

__attribute__((constructor))
static void KERNEL_init(void)
{
    ESP_LOGD(TAG, "constructor: KERNEL_init()");
    glist_initialize(&KERNEL_context.hdl_freed_list);

    for (unsigned I = 0; I < lengthof(KERNEL_context.hdl); I ++)
        glist_push_back(&KERNEL_context.hdl_freed_list, &KERNEL_context.hdl[I]);
}

/***************************************************************************/
/** @implements kernel.h
****************************************************************************/
handle_t KERNEL_handle_get(uint32_t id)
{
    struct KERNEL_hdl *ptr;

    ATOMIC_enter();
    ptr = glist_pop(&KERNEL_context.hdl_freed_list);

    if (! ptr)
    {
        struct KERNEL_hdl *blocks =
            KERNEL_malloc(sizeof(struct KERNEL_hdl) * DYNAMIC_INC_DESCRIPTORS);

        if (blocks)
        {
            for (int I = 1; I < DYNAMIC_INC_DESCRIPTORS; I ++)
                glist_push_back(&KERNEL_context.hdl_freed_list, &blocks[I]);

            ptr = &blocks[0];
        }
        else
        {
            errno = ENOMEM;
            ptr = NULL;
        }
    }
    ATOMIC_leave();

    if (ptr)
    {
        memset(ptr, 0, sizeof(*ptr));

        ptr->cid = (uint8_t)id;
        ptr->flags = HDL_FLAG_SYSMEM;
    }
    return ptr;
}

int KERNEL_handle_release(handle_t hdr)
{
    int retval = 0;

    switch (AsKernelHdl(hdr)->cid)
    {
    /*
    case CID_TCB:
        if (HDL_FLAG_SYSMEM & AsKernelHdl(hdr)->flags)
            KERNEL_mfree(AsTCB(hdr)->stack_base);

        AsTCB(hdr)->flags |= HDL_FLAG_SYSMEM;
        break;
    */

    case CID_FD:
        if (AsFD(hdr)->implement->close)
            retval = AsFD(hdr)->implement->close((int)hdr);

        if (0 == retval)
        {
            /// @filesystem has ext cleanup to do
            /*
            if (NULL != AsFD(hdr)->fs)
                FILESYSTEM_fd_cleanup((int)hdr);

            ATOMIC_enter();
            {
                /// preparing @recycle read_rdy hdr
                ///     .not need to free it when ready_rdy is created by INITIALIZER
                if ((INVALID_HANDLE != AsFD(hdr)->read_rdy) &&
                    (HDL_FLAG_SYSMEM & AsKernelHdl(AsFD(hdr)->read_rdy)->flags))
                {
                    AsKernelHdl(AsFD(hdr)->read_rdy)->flags |= HDL_FLAG_DESTROYING;
                    glist_push_back(&KERNEL_context.hdl_destroying_list, AsFD(hdr)->read_rdy);
                }
                /// preparing @recycle write_rdy hdr
                ///     .not need to free it when write_rdy is created by INITIALIZER
                if ((INVALID_HANDLE != AsFD(hdr)->write_rdy) &&
                    (HDL_FLAG_SYSMEM & AsKernelHdl(AsFD(hdr)->write_rdy)->flags))
                {
                    AsKernelHdl(AsFD(hdr)->write_rdy)->flags |= HDL_FLAG_DESTROYING;
                    glist_push_back(&KERNEL_context.hdl_destroying_list, AsFD(hdr)->write_rdy);
                }
            }
            ATOMIC_leave();
            */
        }
        else
            retval = errno;
        break;

    default:
        if (0 == (CID_SYNCOBJS & AsKernelHdl(hdr)->cid))
        {
            // unknown objects
            retval = EINVAL;
        }
        else if (HDL_FLAG_SYSMEM & AsKernelHdl(hdr)->flags)
        {
            // not possiable to free when syocobjs is created by INITIALIZER
            //  this is error but..the memory should be remain stable, bcuz nothing dangers happens
            retval = EFAULT;
        }
        break;
    }

    if (0 == retval)
    {
        ATOMIC_enter();
        {
            AsKernelHdl(hdr)->flags |= HDL_FLAG_DESTROYING;
            // TODO: hdl_destroying_list
            glist_push_back(&KERNEL_context.hdl_freed_list, hdr);
        }
        ATOMIC_leave();
    }
    return retval;
}

int KERNEL_createfd(uint16_t const TAG, struct FD_implement const *implement, void *ext)
{
    struct KERNEL_fd *fd = KERNEL_handle_get(CID_FD);

    if (INVALID_HANDLE != fd)
    {
        fd->tag = TAG;
        fd->implement = implement;
        fd->ext = ext;

        return (int)fd;
    }
    else
        return -1;
}

void *KERNEL_malloc(uint32_t size)
{
    return malloc(size);
}

void KERNEL_mfree(void *ptr)
{
    free(ptr);
}

void *KERNEL_mallocz(uint32_t size)
{
    void *ptr = KERNEL_malloc(size);

    if (ptr)
        memset(ptr, 0, size);
    return ptr;
}
