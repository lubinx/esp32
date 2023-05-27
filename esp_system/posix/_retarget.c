#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#include <sys/errno.h>
#include <sys/lock.h>
#include <sys/mutex.h>
#include <sys/reent.h>
#include <sys/stat.h>

#include "soc.h"
#include "esp_log.h"

#include "esp_rom_caps.h"
#include "esp_heap_caps_init.h"

/****************************************************************************
 * @public
*****************************************************************************/
struct _reent *_global_impure_ptr;
void *syscall_table_ptr;

/****************************************************************************
 *  @internal
*****************************************************************************/
struct __lock
{
    mutex_t mutex;
};

// static const struct syscall_stub_table __stub_table;
static struct _reent __reent = {0};

// locks
struct __lock    __lock___sinit_recursive_mutex     = {0};
struct __lock    __lock___malloc_recursive_mutex    = {0};
struct __lock    __lock___env_recursive_mutex       = {0};
struct __lock    __lock___sfp_recursive_mutex       = {0};
struct __lock    __lock___atexit_recursive_mutex    = {0};
struct __lock    __lock___at_quick_exit_mutex       = {0};
struct __lock    __lock___tz_mutex                  = {0};
struct __lock    __lock___dd_hash_mutex             = {0};
struct __lock    __lock___arc4random_mutex          = {0};

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

    static struct __lock    idf_common_mutex = {0};
    static struct __lock    idf_common_recursive_mutex = {0};
#endif

/****************************************************************************
 *  @implements
*****************************************************************************/
void __retarget_init(void)
{
    extern void KERNEL_init(void);
    KERNEL_init();

    mutex_init(&__lock___sinit_recursive_mutex.mutex, MUTEX_FLAG_RECURSIVE);
    mutex_init(&__lock___sfp_recursive_mutex.mutex, MUTEX_FLAG_RECURSIVE);
    mutex_init(&__lock___env_recursive_mutex.mutex, MUTEX_FLAG_RECURSIVE);
    mutex_init(&__lock___malloc_recursive_mutex.mutex, MUTEX_FLAG_RECURSIVE);
    mutex_init(&__lock___atexit_recursive_mutex.mutex, MUTEX_FLAG_RECURSIVE);

    mutex_init(&__lock___at_quick_exit_mutex.mutex, MUTEX_FLAG_NORMAL);
    mutex_init(&__lock___tz_mutex.mutex, MUTEX_FLAG_NORMAL);
    mutex_init(&__lock___dd_hash_mutex.mutex, MUTEX_FLAG_NORMAL);
    mutex_init(&__lock___arc4random_mutex.mutex, MUTEX_FLAG_NORMAL);

    #if ESP_ROM_HAS_RETARGETABLE_LOCKING
        mutex_init(&idf_common_recursive_mutex.mutex, MUTEX_FLAG_RECURSIVE);
        mutex_init(&idf_common_mutex.mutex, MUTEX_FLAG_NORMAL);
    #endif

    _global_impure_ptr = &__reent;
    syscall_table_ptr= NULL;        // it has been deprecated and removed from our implement

    heap_caps_init();

    extern void __FILESYSTEM_introduce(void);
    __FILESYSTEM_introduce();
}

int setvbuf(FILE *fp, char *buffer, int mode, size_t size)
{
    ARG_UNUSED(fp, buffer, mode, size);
    return ENOSYS;
}

int _getpid_r(struct _reent *r)
{
    ARG_UNUSED(r);
    return 0;
}

int _kill_r(struct _reent *r, int pid, int sig)
{
    ARG_UNUSED(r, pid, sig);
    return 0;
}

void __assert_func(char const *file, int line, char const *func, char const *failedexpr)
{
    ESP_LOGE("assertion", "\"%s\" failed\n\tfile \"%s\", line %d%s%s\n",
        failedexpr, file, line, func ? ", function: " : "", func ? func : "");

    __BKPT(0);
    exit(EFAULT);
}

void _exit(int status)
{
#ifdef NDEBUG
    SOC_reset();
#else
    ESP_LOGI("exit", "code %d", status);
#endif
    while (1);
}

void abort(void)
{
#if ! defined(NDEBUG)
    ESP_LOGE("abort", "PC 0x%p on core %u", (void *)(__builtin_return_address(0) - 3), __get_CORE_ID());
#endif
    __BKPT(0);
    exit(EFAULT);
}

int atexit(void (*function)(void))
{
    ARG_UNUSED(function);
    return 0;
}

/****************************************************************************
 *  @implements: heap
*****************************************************************************/
/*
 These contain the business logic for the malloc() and realloc() implementation. Because of heap tracing
 wrapping reasons, we do not want these to be a public api, however, so they're not defined publicly.
*/
extern void *heap_caps_malloc_default(size_t size);
extern void *heap_caps_realloc_default(void *ptr, size_t size);

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
    ARG_UNUSED(r);
    return heap_caps_malloc_default(size);
}

void _free_r(struct _reent *r, void *ptr)
{
    ARG_UNUSED(r);
    heap_caps_free(ptr);
}

void *_realloc_r(struct _reent *r, void *ptr, size_t size)
{
    ARG_UNUSED(r);
    return heap_caps_realloc_default(ptr, size);
}

void *_calloc_r(struct _reent *r, size_t nmemb, size_t size)
{
    ARG_UNUSED(r);
    return calloc(nmemb, size);
}

void *memalign(size_t alignment, size_t n)
{
    return heap_caps_aligned_alloc(alignment, n, MALLOC_CAP_DEFAULT);
}

int posix_memalign(void **out_ptr, size_t alignment, size_t size)
{
    if (size == 0)
    {
        /* returning NULL for zero size is allowed, don't treat this as an error */
        *out_ptr = NULL;
        return 0;
    }
    else
    {
        *out_ptr = heap_caps_aligned_alloc(alignment, size, MALLOC_CAP_DEFAULT);
        if (*out_ptr)
            return 0;
        else
            return ENOMEM;
    }
}

/****************************************************************************
 * @implements: locks
*****************************************************************************/
void __retarget_lock_init(_LOCK_T *lock)
{
    *lock = (_LOCK_T)mutex_create(MUTEX_FLAG_NORMAL);
}

void __retarget_lock_init_recursive(_LOCK_T *lock)
{
    *lock = (_LOCK_T)mutex_create(MUTEX_FLAG_RECURSIVE);
}

void __retarget_lock_close(_LOCK_T lock)
{
    mutex_destroy(&lock->mutex);
}

void __retarget_lock_close_recursive(_LOCK_T lock)
    __attribute__((alias("__retarget_lock_close")));

void __retarget_lock_acquire(_LOCK_T lock)
{
#if ESP_ROM_HAS_RETARGETABLE_LOCKING
    if (ROM_MUTEX_MAGIC == *(uintptr_t *)lock)
        lock = &idf_common_mutex;
#endif
    mutex_lock(&lock->mutex);
}

void __retarget_lock_acquire_recursive(_LOCK_T lock)
{
#if ESP_ROM_HAS_RETARGETABLE_LOCKING
    if (ROM_MUTEX_MAGIC == *(uintptr_t *)lock)
        lock = &idf_common_recursive_mutex;
#endif
    mutex_lock(&lock->mutex);
}

int __retarget_lock_try_acquire(_LOCK_T lock)
{
#if ESP_ROM_HAS_RETARGETABLE_LOCKING
    if (ROM_MUTEX_MAGIC == *(uintptr_t *)lock)
        lock = &idf_common_mutex;
#endif
    return mutex_trylock(&lock->mutex, 0);
}

int __retarget_lock_try_acquire_recursive(_LOCK_T lock)
{
#if ESP_ROM_HAS_RETARGETABLE_LOCKING
    if (ROM_MUTEX_MAGIC == *(uintptr_t *)lock)
        lock = &idf_common_recursive_mutex;
#endif
    return mutex_trylock(&lock->mutex, 0);
}

void __retarget_lock_release(_LOCK_T lock)
{
#if ESP_ROM_HAS_RETARGETABLE_LOCKING
    if (ROM_MUTEX_MAGIC == *(uintptr_t *)lock)
        lock = &idf_common_mutex;
#endif
    mutex_unlock(&lock->mutex);
}

void __retarget_lock_release_recursive(_LOCK_T lock)
{
#if ESP_ROM_HAS_RETARGETABLE_LOCKING
    if (ROM_MUTEX_MAGIC == *(uintptr_t *)lock)
        lock = &idf_common_recursive_mutex;
#endif
    mutex_unlock(&lock->mutex);
}
