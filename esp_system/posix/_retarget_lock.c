#include <string.h>
#include <stdlib.h>

#include <sys/lock.h>
#include <sys/errno.h>

#include "esp_log.h"
#include "esp_heap_caps.h"

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

// requirement checking
static_assert(sizeof(struct __lock) != sizeof(StaticSemaphore_t), "sizeof(struct __lock) != sizeof(StaticSemaphore_t)");
static_assert(configSUPPORT_STATIC_ALLOCATION, "FreeRTOS should be configured with static allocation support");

static struct __lock    __sinit_recursive_mutex = {0};
static struct __lock    __malloc_recursive_mutex = {0};
static struct __lock    __env_recursive_mutex = {0};
static struct __lock    __sfp_recursive_mutex = {0};
static struct __lock    __atexit_recursive_mutex = {0};
static struct __lock    __at_quick_exit_mutex = {0};
static struct __lock    __tz_mutex = {0};
static struct __lock    __dd_hash_mutex = {0};
static struct __lock    __arc4random_mutex = {0};

#if ESP_ROM_HAS_RETARGETABLE_LOCKING
    /* C3 and S3 ROMs are built without Newlib static lock symbols exported, and
    * with an extra level of _LOCK_T indirection in mind.
    * The following is a workaround for this:
    * - on startup, we call esp_rom_newlib_init_common_mutexes to set
    *   the two mutex pointers to magic values.
    * - if in __retarget_lock_acquire*, we check if the argument dereferences
    *   to the magic value. If yes, we lock the correct mutex defined in the app,
    *   instead.
    * Casts from &StaticSemaphore_t to _LOCK_T are okay because _LOCK_T
    * (which is SemaphoreHandle_t) is a pointer to the corresponding
    * StaticSemaphore_t structure.
    */
    #define ROM_MUTEX_MAGIC             0xbb10c433

    static struct __lock    s_common_mutex = {0};
    static struct __lock    s_common_recursive_mutex = {0};
#endif

/* somehow esp-idf libc.a already defined these, this is how to override it */
extern struct __lock    __lock___sinit_recursive_mutex  __attribute__((alias("__sinit_recursive_mutex")));
extern struct __lock    __lock___malloc_recursive_mutex __attribute__((alias("__malloc_recursive_mutex")));
extern struct __lock    __lock___env_recursive_mutex    __attribute__((alias("__env_recursive_mutex")));
extern struct __lock    __lock___sfp_recursive_mutex    __attribute__((alias("__sfp_recursive_mutex")));
extern struct __lock    __lock___atexit_recursive_mutex __attribute__((alias("__atexit_recursive_mutex")));
extern struct __lock    __lock___at_quick_exit_mutex    __attribute__((alias("__at_quick_exit_mutex")));
extern struct __lock    __lock___tz_mutex               __attribute__((alias("__tz_mutex")));
extern struct __lock    __lock___dd_hash_mutex          __attribute__((alias("__dd_hash_mutex")));
extern struct __lock    __lock___arc4random_mutex       __attribute__((alias("__arc4random_mutex")));

void __LOCK_retarget(void)
{
    xSemaphoreCreateRecursiveMutexStatic((void *)&__lock___sinit_recursive_mutex.__pad);
    xSemaphoreCreateRecursiveMutexStatic((void *)&__lock___sfp_recursive_mutex.__pad);
    xSemaphoreCreateRecursiveMutexStatic((void *)&__lock___env_recursive_mutex.__pad);
    xSemaphoreCreateRecursiveMutexStatic((void *)&__lock___malloc_recursive_mutex.__pad);
    xSemaphoreCreateRecursiveMutexStatic((void *)&__lock___atexit_recursive_mutex.__pad);

    xSemaphoreCreateMutexStatic((void *)&__lock___at_quick_exit_mutex.__pad);
    xSemaphoreCreateMutexStatic((void *)&__lock___tz_mutex.__pad);
    xSemaphoreCreateMutexStatic((void *)&__lock___dd_hash_mutex.__pad);
    xSemaphoreCreateMutexStatic((void *)&__lock___arc4random_mutex.__pad);

    #if ESP_ROM_HAS_RETARGETABLE_LOCKING
        xSemaphoreCreateMutexStatic((void *)&s_common_mutex.__pad);
        xSemaphoreCreateMutexStatic((void *)&s_common_recursive_mutex.__pad);
    #endif
}

/****************************************************************************
 * @implements: static locks
*****************************************************************************/
void libc_lock_init(_LOCK_T *lock)
{
    *lock = (void *)xSemaphoreCreateMutex();
}

void libc_lock_init_recursive(_LOCK_T *lock)
{
    *lock = (void *)xSemaphoreCreateRecursiveMutex();
}

void libc_lock_sinit(_LOCK_T lock)
{
    xSemaphoreCreateMutexStatic((void *)&lock->__pad);
}

void libc__lock_sinit_recursive(_LOCK_T lock)
{
    xSemaphoreCreateRecursiveMutexStatic((void *)&lock->__pad);
}

void libc_lock_close(_LOCK_T lock)
{
    vSemaphoreDelete(lock->__pad);
}

void libc_lock_acquire(_LOCK_T lock)
{
    if (0 != __get_IPSR())
    {
        BaseType_t higher_task_woken = false;
        if (! xSemaphoreTakeFromISR((void *)&lock->__pad, &higher_task_woken))
        {
            assert(higher_task_woken);
            portYIELD_FROM_ISR();
        }
    }
    else
        xSemaphoreTake((void *)&lock->__pad, portMAX_DELAY);
}

void libc_lock_acquire_recursive(_LOCK_T lock)
{
    xSemaphoreTakeRecursive((void *)&lock->__pad, portMAX_DELAY);
}


int libc_lock_try_acquire(_LOCK_T lock)
{
    if (pdTRUE == xSemaphoreTake((void *)&lock->__pad, 0))
        return 0;
    else
        return EBUSY;
}

int libc_lock_try_acquire_recursive(_LOCK_T lock)
{
    if (pdTRUE == xSemaphoreTakeRecursive((void *)&lock->__pad, 0))
        return 0;
    else
        return EBUSY;
}

void libc_lock_release(_LOCK_T lock)
{
    if (0 != __get_IPSR())
    {
        BaseType_t higher_task_woken = false;
        xSemaphoreGiveFromISR((void *)&lock->__pad, &higher_task_woken);

        if (higher_task_woken)
            portYIELD_FROM_ISR();
    }
    else
        xSemaphoreGive((void *)&lock->__pad);
}

void libc_lock_release_recursive(_LOCK_T lock)
{
    xSemaphoreGiveRecursive((void *)&lock->__pad);
}

void __retarget_lock_init(_LOCK_T *lock)
    __attribute__((alias("libc_lock_init")));

void __retarget_lock_init_recursive(_LOCK_T *lock)
    __attribute__((alias("libc_lock_init_recursive")));

void libc_lock_close_recursive(_LOCK_T lock)
    __attribute__((alias("libc_lock_close")));

void __retarget_lock_close(_LOCK_T lock)
    __attribute__((alias("libc_lock_close")));

void __retarget_lock_close_recursive(_LOCK_T lock)
    __attribute__((alias("libc_lock_close")));

/****************************************************************************
 * @implements: newlib retargeting
*****************************************************************************/
void __retarget_lock_acquire(_LOCK_T lock)
{
#if ESP_ROM_HAS_RETARGETABLE_LOCKING
    if (ROM_MUTEX_MAGIC == *(int*)lock)
        lock = &s_common_mutex;
#endif
    _lock_acquire(&lock);
}

void __retarget_lock_acquire_recursive(_LOCK_T lock)
{
#if ESP_ROM_HAS_RETARGETABLE_LOCKING
    if (ROM_MUTEX_MAGIC == *(int *)lock)
        lock = &s_common_recursive_mutex;
#endif
    _lock_acquire_recursive(&lock);
}

int __retarget_lock_try_acquire(_LOCK_T lock)
{
#if ESP_ROM_HAS_RETARGETABLE_LOCKING
    if (ROM_MUTEX_MAGIC == *(int *)lock)
        lock = &s_common_mutex;
#endif
    return _lock_try_acquire(&lock);
}

int __retarget_lock_try_acquire_recursive(_LOCK_T lock)
{
#if ESP_ROM_HAS_RETARGETABLE_LOCKING
    if (ROM_MUTEX_MAGIC == *(int *)lock)
        lock = &s_common_recursive_mutex;
#endif
    return _lock_try_acquire_recursive(&lock);
}

void __retarget_lock_release(_LOCK_T lock)
{
#if ESP_ROM_HAS_RETARGETABLE_LOCKING
    if (ROM_MUTEX_MAGIC == *(int *)lock)
        lock = &s_common_mutex;
#endif
    _lock_release(&lock);
}

void __retarget_lock_release_recursive(_LOCK_T lock)
{
#if ESP_ROM_HAS_RETARGETABLE_LOCKING
    if (ROM_MUTEX_MAGIC == *(int*)lock)
        lock = &s_common_recursive_mutex;
#endif
    _lock_release_recursive(&lock);
}

/*
 These contain the business logic for the malloc() and realloc() implementation. Because of heap tracing
 wrapping reasons, we do not want these to be a public api, however, so they're not defined publicly.
*/
extern void *heap_caps_malloc_default( size_t size );
extern void *heap_caps_realloc_default( void *ptr, size_t size );

void *malloc(size_t size)
{
    return heap_caps_malloc_default(size);
}

void *calloc(size_t nmemb, size_t size)
{
    size_t size_bytes;
    if (__builtin_mul_overflow(nmemb, size, &size_bytes))
        return NULL;

    void *ptr = heap_caps_malloc_default(size_bytes);
    if (ptr)
        memset(ptr, 0, size_bytes);
    return ptr;
}

void *realloc(void *ptr, size_t size)
{
    return heap_caps_realloc_default(ptr, size);
}

void free(void *ptr)
{
    heap_caps_free(ptr);
}

void *_malloc_r(struct _reent *r, size_t size)
{
    return heap_caps_malloc_default(size);
}

void _free_r(struct _reent *r, void *ptr)
{
    heap_caps_free(ptr);
}

void *_realloc_r(struct _reent *r, void *ptr, size_t size)
{
    return heap_caps_realloc_default(ptr, size);
}

void *_calloc_r(struct _reent *r, size_t nmemb, size_t size)
{
    return calloc(nmemb, size);
}

void *memalign(size_t alignment, size_t n)
{
    return heap_caps_aligned_alloc(alignment, n, MALLOC_CAP_DEFAULT);
}

int posix_memalign(void **out_ptr, size_t alignment, size_t size)
{
    if (size == 0) {
        /* returning NULL for zero size is allowed, don't treat this as an error */
        *out_ptr = NULL;
        return 0;
    }
    void *result = heap_caps_aligned_alloc(alignment, size, MALLOC_CAP_DEFAULT);
    if (result != NULL) {
        /* Modify output pointer only on success */
        *out_ptr = result;
        return 0;
    }
    /* Note: error returned, not set via errno! */
    return ENOMEM;
}

/****************************************************************************
 * @implements: esp-idf
*****************************************************************************/
int __esp_lock_impl(_LOCK_T *lock, int (*libc_lock_func)(_LOCK_T lock), char const *__function__)
{
    if (taskSCHEDULER_NOT_STARTED == xTaskGetSchedulerState())
    {
        /**
         *  somehow they make this happen in esp-idf source code
         *      this is ...ok when freertos task scheduler is not running, none thread was dispatching
         *  REVIEW: add a atomic lock?
        */
        ESP_LOGW("libc", "%s() before freertos scheduler startup...%p", __function__, *lock);
    }
    else
    {
        SemaphoreHandle_t hdl = (SemaphoreHandle_t)(*lock);
        assert(NULL != hdl);
        /*
        if (! hdl)
        {
            ESP_LOGE("libc", "lock acquire CREATE... %p", lock);

            hdl = xSemaphoreCreateMutex();
            assert(NULL != hdl);

            *lock = (void *)hdl;
        }
        */
        return libc_lock_func((void *)hdl);
    }
}
