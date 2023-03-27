#include <string.h>

#include <sys/errno.h>
#include <esp_log.h>

#include <rtos/kernel.h>

static char const *TAG = "kernel";
#define DYNAMIC_INC_DESCRIPTORS         (1024 / sizeof(sizeof(struct KERNEL_hdl)))

struct KERNEL_context_t
{
    spinlock_t atomic;

    glist_t mounted_dev;

    glist_t hdl_freed_list;
    glist_t hdl_destroying_list;
    struct KERNEL_hdl hdl[4096 / sizeof(struct KERNEL_hdl)];
};
struct KERNEL_context_t KERNEL_context = {0};

/***************************************************************************/
/** constructor
****************************************************************************/
void KERNEL_init(void)
{
    spinlock_init(&KERNEL_context.atomic);

    glist_initialize(&KERNEL_context.mounted_dev);
    glist_initialize(&KERNEL_context.hdl_freed_list);
    glist_initialize(&KERNEL_context.hdl_destroying_list);

    for (unsigned I = 0; I < lengthof(KERNEL_context.hdl); I++)
        glist_push_back(&KERNEL_context.hdl_freed_list, &KERNEL_context.hdl[I]);
}

/***************************************************************************/
/** @implements kernel.h
****************************************************************************/
handle_t KERNEL_handle_get(uint8_t cid)
{
    struct KERNEL_hdl *ptr;

    spin_lock(&KERNEL_context.atomic);
    ptr = glist_pop(&KERNEL_context.hdl_freed_list);

    if (! ptr)
    {
        struct KERNEL_hdl *blocks =
            KERNEL_malloc(sizeof(struct KERNEL_hdl) * DYNAMIC_INC_DESCRIPTORS);

        if (blocks)
        {
            for (int I = 1; I < DYNAMIC_INC_DESCRIPTORS; I++)
                glist_push_back(&KERNEL_context.hdl_freed_list, &blocks[I]);

            ptr = &blocks[0];
        }
        else
        {
            errno = ENOMEM;
            ptr = NULL;
        }
    }
    spin_unlock(&KERNEL_context.atomic);

    if (ptr)
    {
        memset(ptr, 0, sizeof(*ptr));

        ptr->cid = cid;
        ptr->flags = HDL_FLAG_SYSMEM_MANAGED;
    }
    return ptr;
}

int KERNEL_handle_release(handle_t hdr)
{
    int retval = 0;

    switch (AsKernelHdl(hdr)->cid)
    {
    case CID_TCB:
        break;

    case CID_FD:
        if (AsFD(hdr)->implement->close)
            retval = AsFD(hdr)->implement->close((int)hdr);

        if (0 == retval)
        {
            /// @filesystem has ext cleanup to do
            /*
            if (NULL != AsFD(hdr)->fs)
                FILESYSTEM_fd_cleanup((int)hdr);
            */

            spin_lock(&KERNEL_context.atomic);
            {
                /// preparing @recycle read_rdy hdr
                ///     .not need to free it when ready_rdy is created by INITIALIZER
                if ((INVALID_HANDLE != AsFD(hdr)->read_rdy))
                {
                    AsKernelHdl(AsFD(hdr)->read_rdy)->flags |= HDL_FLAG_DESTROYING;
                    glist_push_back(&KERNEL_context.hdl_destroying_list, AsFD(hdr)->read_rdy);
                }
                /// preparing @recycle write_rdy hdr
                ///     .not need to free it when write_rdy is created by INITIALIZER
                if ((INVALID_HANDLE != AsFD(hdr)->write_rdy))
                {
                    AsKernelHdl(AsFD(hdr)->write_rdy)->flags |= HDL_FLAG_DESTROYING;
                    glist_push_back(&KERNEL_context.hdl_destroying_list, AsFD(hdr)->write_rdy);
                }
            }
            spin_unlock(&KERNEL_context.atomic);
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
        break;
    }

    if (0 == retval)
    {
        spin_lock(&KERNEL_context.atomic);
        {
            AsKernelHdl(hdr)->flags |= HDL_FLAG_DESTROYING;
            glist_push_back(&KERNEL_context.hdl_destroying_list, hdr);
        }
        spin_unlock(&KERNEL_context.atomic);
    }
    return retval;
}

void KERNEL_handle_recycle(void)
{
    if (! glist_is_empty(&KERNEL_context.hdl_destroying_list))
    {
        spin_lock(&KERNEL_context.atomic);
        struct KERNEL_hdl *hdl;

        while (NULL != (hdl = glist_pop(&KERNEL_context.hdl_destroying_list)))
        {
            hdl->cid = CID_FREED;

            if (HDL_FLAG_SYSMEM_MANAGED & hdl->flags)
                glist_push_back(&KERNEL_context.hdl_freed_list, hdl);
        }
        spin_unlock(&KERNEL_context.atomic);
    }
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
