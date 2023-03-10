#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <stdbool.h>
#include <string.h>

#include <sys/errno.h>
#include <sys/reent.h>
#include <sys/stat.h>

#include "soc.h"
#include "esp_log.h"

#include "esp_rom_sys.h"
#include "esp_heap_caps_init.h"

/****************************************************************************
 * @imports
*****************************************************************************/
extern void __LOCK_retarget_init(void);
extern void __FILESYSTEM_init(void);
extern void __IO_retarget(void);

struct _reent *_global_impure_ptr;
extern struct _reent *__getreent(void);     // freertos_tasks_c_additions.h linked by freertos

/****************************************************************************
 *  @internal
*****************************************************************************/
// static const struct syscall_stub_table __stub_table;
static struct _reent __reent = {0};

/****************************************************************************
 *  @implements
*****************************************************************************/
void __libc_retarget_init(void)
{
    heap_caps_init();

    extern void KERNEL_init(void);
    KERNEL_init();

    __LOCK_retarget_init();

    _GLOBAL_REENT = &__reent;
    __sinit(&__reent);

    __FILESYSTEM_init();
    __IO_retarget();

    extern void esp_pthread_init(void);
    esp_pthread_init();
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

void __assert_func(char const *file, int line, char const *func, char const *failedexpr)
{
    ESP_LOGE("assertion", "\"%s\" failed\n\tfile \"%s\", line %d%s%s\n",
        failedexpr, file, line, func ? ", function: " : "", func ? func : "");

    __BKPT(0);
    exit(EFAULT);
}

void _exit(int status)
{
    int core_id = __get_CORE_ID();
    for (uint32_t i = 0; i < SOC_CPU_CORES_NUM; i ++)
    {
        if (i != core_id)
            SOC_acquire_core(i);
    }

    SOC_reset();
    while (1);
}

int _kill_r(struct _reent *r, int pid, int sig)
{
    exit(EFAULT);
}

void abort(void)
{
    ESP_LOGE("signal", "abort() was called at PC 0x%p on core %d", __builtin_return_address(0) - 3, __get_CORE_ID());

    __BKPT(0);
    exit(EFAULT);
}

int atexit(void (*function)(void))
{
    return 0;
}

#define __ENOSYS                        { return __set_errno_r_neg(r, ENOSYS); }
#define __WEAK                          __attribute__((weak))

__WEAK int _system_r(struct _reent *r, char const *str)                             __ENOSYS;

__WEAK int _isatty_r(struct _reent *r, int fd)                                      __ENOSYS;
__WEAK int _open_r(struct _reent *r, char const *path, int flags, int mode)         __ENOSYS;
__WEAK int _close_r(struct _reent *r, int fd)                                       __ENOSYS;
__WEAK int _fcntl_r(struct _reent *r, int fd, int cmd, int arg)                     __ENOSYS
__WEAK int _fstat_r(struct _reent *r, int fd, struct stat *st)                      __ENOSYS;
__WEAK int _stat_r(struct _reent *r, char const *path, struct stat * st)            __ENOSYS;
__WEAK int _link_r(struct _reent *r, const char *n1, const char *n2)                __ENOSYS;
__WEAK int _unlink_r(struct _reent *r, char const *path)                            __ENOSYS;
__WEAK int _rename_r(struct _reent *r, char const *src, char const *dst)            __ENOSYS;

__WEAK off_t _lseek_r(struct _reent *r, int fd, off_t offset, int origin)           __ENOSYS;
__WEAK ssize_t _read_r(struct _reent *r, int fd, void *buf, size_t bufsize)         __ENOSYS;
__WEAK ssize_t _write_r(struct _reent *r, int fd, void const *buf, size_t count)    __ENOSYS;

/****************************************************************************
 *  @implements: heap
*****************************************************************************/
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
 *  @internal
*****************************************************************************/
/* REVIEW: it seems deprecated if we implements all libc retargets
extern void _cleanup_r(struct _reent *r);

extern int _printf_float(struct _reent *rptr, void *pdata,
    FILE * fp, int (*pfunc) (struct _reent *, FILE *, char const *, size_t len), va_list * ap);
extern int _scanf_float(struct _reent *rptr, void *pdata, FILE *fp, va_list *ap);

static void raise_r_stub(struct _reent *rptr)
{
    _raise_r(rptr, 0);
}

static const struct syscall_stub_table __stub_table =
{
    .__getreent = &__getreent,
    ._malloc_r = &_malloc_r,
    ._free_r = &_free_r,
    ._realloc_r = &_realloc_r,
    ._calloc_r = &_calloc_r,
    ._abort = &abort,
    ._system_r = &_system_r,
    ._rename_r = &_rename_r,
    ._times_r = &_times_r,
    ._gettimeofday_r = &_gettimeofday_r,
    ._raise_r = &raise_r_stub,
    ._unlink_r = &_unlink_r,
    ._link_r = &_link_r,
    ._stat_r = &_stat_r,
    ._fstat_r = &_fstat_r,
    ._sbrk_r = &_sbrk_r,
    ._getpid_r = &_getpid_r,
    ._kill_r = &_kill_r,
    ._exit_r = NULL,    // never called in ROM
    ._close_r = &_close_r,
    ._open_r = &_open_r,
    ._write_r = (int (*)(struct _reent *r, int, const void *, int)) &_write_r,
    ._lseek_r = (int (*)(struct _reent *r, int, int, int)) &_lseek_r,
    ._read_r = (int (*)(struct _reent *r, int, void *, int)) &_read_r,
#if ESP_ROM_HAS_RETARGETABLE_LOCKING
    ._retarget_lock_init = &__retarget_lock_init,
    ._retarget_lock_init_recursive = &__retarget_lock_init_recursive,
    ._retarget_lock_close = &__retarget_lock_close,
    ._retarget_lock_close_recursive = &__retarget_lock_close_recursive,
    ._retarget_lock_acquire = &__retarget_lock_acquire,
    ._retarget_lock_acquire_recursive = &__retarget_lock_acquire_recursive,
    ._retarget_lock_try_acquire = &__retarget_lock_try_acquire,
    ._retarget_lock_try_acquire_recursive = &__retarget_lock_try_acquire_recursive,
    ._retarget_lock_release = &__retarget_lock_release,
    ._retarget_lock_release_recursive = &__retarget_lock_release_recursive,
#else
    ._lock_init = &_lock_init,
    ._lock_init_recursive = &_lock_init_recursive,
    ._lock_close = &_lock_close,
    ._lock_close_recursive = &_lock_close_recursive,
    ._lock_acquire = &_lock_acquire,
    ._lock_acquire_recursive = &_lock_acquire_recursive,
    ._lock_try_acquire = &_lock_try_acquire,
    ._lock_try_acquire_recursive = &_lock_try_acquire_recursive,
    ._lock_release = &_lock_release,
    ._lock_release_recursive = &_lock_release_recursive,
#endif
// REVIEW: link with stdandard-newlib is hardcoded inside esp-idf
//  SOO...wtf is nano printf format?
#ifdef CONFIG_NEWLIB_NANO_FORMAT
    ._printf_float = &_printf_float,
    ._scanf_float = &_scanf_float,
#else
    ._printf_float = NULL,
    ._scanf_float = NULL,
#endif
// REVIEW: why they make these simply things so confuse?
#if CONFIG_IDF_TARGET_ESP32S3 || CONFIG_IDF_TARGET_ESP32C3 || CONFIG_IDF_TARGET_ESP32H4 \
    || CONFIG_IDF_TARGET_ESP32C2 || CONFIG_IDF_TARGET_ESP32C6
    // TODO IDF-2570 : mark that this assert failed in ROM, to avoid confusion between IDF & ROM
        // assertion failures (as function names & source file names will be similar)
    .__assert_func = __assert_func,
    .__sinit = &__sinit,
    ._cleanup_r = &_cleanup_r,
#endif
};
*/
