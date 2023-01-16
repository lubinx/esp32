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
