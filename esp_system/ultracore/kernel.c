#include <ultracore/kernel.h>
#include <string.h>
#include <sys/errno.h>

#include "esp_cpu.h"
#include "esp_log.h"

#if defined(__XTENSA__)
    #include "xtensa/xtruntime.h"
    #include "xt_utils.h"
    #include "spinlock.h"
#elif defined(__riscv)
#else
    #pragma GCC error "pthread: unsupported arch"
#endif

#define DYNAMIC_INC_DESCRIPTORS         (1024 / sizeof(sizeof(struct KERNEL_hdl)))

struct KERNEL_context_t
{
    struct KERNEL_hdl hdl[4096 / sizeof(struct KERNEL_hdl)];
    glist_t hdl_freed_list;
};
struct KERNEL_context_t KERNEL_context;

static __attribute__((constructor))
void KERNEL_init(void)
{
    glist_initialize(&KERNEL_context.hdl_freed_list);

    for (unsigned I = 0; I < lengthof(KERNEL_context.hdl); I ++)
        glist_push_back(&KERNEL_context.hdl_freed_list, &KERNEL_context.hdl[I]);
}

/***************************************************************************/
/** @implements atomic.h / spinlock.h
****************************************************************************/
void ATOMIC_enter(void)
{
}

void ATOMIC_leave(void)
{
}

void spinlock_initialize(spinlock_t *lock)
{
    lock->cpuid = 0;
    lock->intr_status = 0;
    lock->lock_count = 0;
}

bool spinlock_acquire(spinlock_t *lock, uint32_t timeout)
{
    if (SPINLOCK_WAIT_FOREVER != timeout)
        ESP_LOGE("spinlock", "spinlock timeout should be SPINLOCK_WAIT_FOREVER");

    bool lock_set;

    #ifdef __XTENSA__
        uint32_t irq_status = XTOS_SET_INTLEVEL(XCHAL_EXCM_LEVEL);
        uint32_t core_id = xt_utils_get_raw_core_id() + 1;

        // The caller is already the owner of the lock. Simply increment the nesting count
        if (lock->cpuid == core_id)
        {
            lock->lock_count ++;
            XTOS_RESTORE_INTLEVEL(irq_status);
            return true;
        }
    #endif

    if (SPINLOCK_WAIT_FOREVER == timeout)
    {
        while (! esp_cpu_compare_and_set(&lock->cpuid, 0, core_id));
        lock_set = true;
    }
    else
        lock_set = esp_cpu_compare_and_set(&lock->cpuid, 0, core_id);

    if (lock_set)
        lock->lock_count ++;

    #ifdef __XTENSA__
        XTOS_RESTORE_INTLEVEL(irq_status);
    #endif

    return lock_set;
}

void spinlock_release(spinlock_t *lock)
{
    #ifdef __XTENSA__
        uint32_t irq_status = XTOS_SET_INTLEVEL(XCHAL_EXCM_LEVEL);
        assert(lock->cpuid == xt_utils_get_raw_core_id() + 1);
    #endif

    if (0 == __sync_sub_and_fetch(&lock->lock_count, 1))
        lock->cpuid = 0;

    #ifdef __XTENSA__
        XTOS_RESTORE_INTLEVEL(irq_status);
    #endif
}

/***************************************************************************/
/** kernel
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
