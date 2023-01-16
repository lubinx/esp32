/*
 * SPDX-FileCopyrightText: 2015-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdlib.h>
#include <errno.h>
#include <stdbool.h>
#include <string.h>

#include <sys/errno.h>
#include <sys/reent.h>
#include <sys/stat.h>

#include "esp_log.h"
#include "esp_rom_caps.h"
#include "esp_newlib.h"

#if CONFIG_IDF_TARGET_ESP32
    #include "esp32/rom/libc_stubs.h"
#elif CONFIG_IDF_TARGET_ESP32S2
    #include "esp32s2/rom/libc_stubs.h"
#elif CONFIG_IDF_TARGET_ESP32S3
    #include "esp32s3/rom/libc_stubs.h"
#elif CONFIG_IDF_TARGET_ESP32C3
    #include "esp32c3/rom/libc_stubs.h"
#elif CONFIG_IDF_TARGET_ESP32H4
    #include "esp32h4/rom/libc_stubs.h"
#elif CONFIG_IDF_TARGET_ESP32C2
    #include "esp32c2/rom/libc_stubs.h"
#elif CONFIG_IDF_TARGET_ESP32C6
    #include "esp32c6/rom/libc_stubs.h"
#endif

#include "freertos/FreeRTOS.h"

/****************************************************************************
 * @imports
*****************************************************************************/
extern void __LOCK_retarget(void);
extern void __FILESYSTEM_init(void);
extern void __IO_retarget(void);

extern int _printf_float(struct _reent *rptr, void *pdata,
    FILE * fp, int (*pfunc) (struct _reent *, FILE *, const char *, size_t len), va_list * ap);
extern int _scanf_float(struct _reent *rptr, void *pdata, FILE *fp, va_list *ap);

extern void _cleanup_r(struct _reent* r);
extern struct _reent *__getreent(void);     // freertos_tasks_c_additions.h linked by freertos

/****************************************************************************
 *  @internal
*****************************************************************************/
static void raise_r_stub(struct _reent *rptr);

static const struct syscall_stub_table __stub_table;
static struct _reent __reent;

/****************************************************************************
 *  @implements
*****************************************************************************/
void esp_newlib_init(void)
{
    #if CONFIG_IDF_TARGET_ESP32
        syscall_table_ptr_pro = syscall_table_ptr_app = &__stub_table;
    #elif CONFIG_IDF_TARGET_ESP32S2
        syscall_table_ptr_pro = &__stub_table;
    #elif CONFIG_IDF_TARGET_ESP32S3 || CONFIG_IDF_TARGET_ESP32C3 || CONFIG_IDF_TARGET_ESP32H4 \
        || CONFIG_IDF_TARGET_ESP32C2 || CONFIG_IDF_TARGET_ESP32C6
        syscall_table_ptr = (void *)&__stub_table;
    #endif

    _GLOBAL_REENT = &__reent;
    esp_vfs_console_register();

    #if defined(CONFIG_VFS_SUPPORT_IO) && !defined(CONFIG_ESP_CONSOLE_NONE)
        esp_reent_init(_GLOBAL_REENT);

        static char const *default_stdio_dev = "/dev/console/";
        _GLOBAL_REENT->_stdin  = fopen(default_stdio_dev, "r");
        _GLOBAL_REENT->_stdout = fopen(default_stdio_dev, "w");
        _GLOBAL_REENT->_stderr = fopen(default_stdio_dev, "w");
    #else
        _REENT_SMALL_CHECK_INIT(_GLOBAL_REENT);
    #endif

    __LOCK_retarget();
    __IO_retarget();
    // __FILESYSTEM_init();

    extern void esp_newlib_time_init(void);
    esp_newlib_time_init();

    extern void esp_pthread_init(void);
    esp_pthread_init();
}

void esp_reent_init(struct _reent* r)
{
    memset(r, 0, sizeof(*r));

    r->_stdout = _GLOBAL_REENT->_stdout;
    r->_stderr = _GLOBAL_REENT->_stderr;
    r->_stdin  = _GLOBAL_REENT->_stdin;

    r->__cleanup = &_cleanup_r;
    r->__sdidinit = 1;
    r->__sglue._next = NULL;
    r->__sglue._niobs = 0;
    r->__sglue._iobs = NULL;
}

void esp_reent_cleanup(void)
{
    struct _reent* r = __getreent();
    /* Clean up storage used by mprec functions */
    if (r->_mp)
    {
        if (_REENT_MP_FREELIST(r))
        {
            for (unsigned int i = 0; i < _Kmax; ++i)
            {
                struct _Bigint *cur, *next;
                next = _REENT_MP_FREELIST(r)[i];
                while (next) {
                    cur = next;
                    next = next->_next;
                    free(cur);
                }
            }
        }
        free(_REENT_MP_FREELIST(r));
        free(_REENT_MP_RESULT(r));
    }

    /* Clean up "glue" (lazily-allocated FILE objects) */
    struct _glue* prev = &_GLOBAL_REENT->__sglue;
    for (struct _glue* cur = _GLOBAL_REENT->__sglue._next; cur != NULL;)
    {
        if (cur->_niobs == 0)
        {
            cur = cur->_next;
            continue;
        }
        bool has_open_files = false;
        for (int i = 0; i < cur->_niobs; ++i)
        {
            FILE* fp = &cur->_iobs[i];
            if (fp->_flags != 0)
            {
                has_open_files = true;
                break;
            }
        }
        if (has_open_files)
        {
            prev = cur;
            cur = cur->_next;
            continue;
        }

        struct _glue* next = cur->_next;
        prev->_next = next;
        free(cur);
        cur = next;
    }

    /* Clean up various other buffers */
    free(r->_mp);
    r->_mp = NULL;
    free(r->_r48);
    r->_r48 = NULL;
    free(r->_localtime_buf);
    r->_localtime_buf = NULL;
    free(r->_asctime_buf);
    r->_asctime_buf = NULL;
}

int _getpid_r(struct _reent *r)
{
    ARG_UNUSED(r);
    return 0;
}

#define __ENOSYS                        { return __set_errno_neg(r, ENOSYS); }
#define __WEAK                          __attribute__((weak))

__WEAK int _system_r(struct _reent *r, const char *str)
    // __attribute__((alias("syscall_not_implemented")));
{
    ESP_LOGE("syscall", "_system_r()");
    while(1);
}

__WEAK int _isatty_r(struct _reent *r, int fd)                                   __ENOSYS;
__WEAK int _open_r(struct _reent *r, const char *path, int flags, int mode)      __ENOSYS;
__WEAK int _close_r(struct _reent *r, int fd)                                    __ENOSYS;
__WEAK int _fstat_r(struct _reent *r, int fd, struct stat *st)                   __ENOSYS;
__WEAK int _stat_r(struct _reent *r, const char *path, struct stat * st)         __ENOSYS;
__WEAK int _link_r(struct _reent *r, const char* n1, const char* n2)             __ENOSYS;
__WEAK int _unlink_r(struct _reent *r, const char *path)                         __ENOSYS;
__WEAK int _rename_r(struct _reent *r, const char *src, const char *dst)         __ENOSYS;
__WEAK ssize_t _read_r(struct _reent *r, int fd, void * dst, size_t size)        __ENOSYS;
__WEAK ssize_t _write_r(struct _reent *r, int fd, void const *data, size_t size) __ENOSYS;
__WEAK off_t _lseek_r(struct _reent *r, int fd, off_t size, int mode)            __ENOSYS;

/****************************************************************************
 *  @internal
*****************************************************************************/
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
#ifdef CONFIG_NEWLIB_NANO_FORMAT
    ._printf_float = &_printf_float,
    ._scanf_float = &_scanf_float,
#else
    ._printf_float = NULL,
    ._scanf_float = NULL,
#endif
#if CONFIG_IDF_TARGET_ESP32S3 || CONFIG_IDF_TARGET_ESP32C3 || CONFIG_IDF_TARGET_ESP32H4 \
    || CONFIG_IDF_TARGET_ESP32C2 || CONFIG_IDF_TARGET_ESP32C6
    /*
        TODO IDF-2570 : mark that this assert failed in ROM, to avoid confusion between IDF & ROM
            assertion failures (as function names & source file names will be similar)
    */
    .__assert_func = __assert_func,
    /*
        We don't expect either ROM code or IDF to ever call __sinit, so it's implemented as abort() for now.
            esp_reent_init() does this job inside IDF.
        Kept in the syscall table in case we find a need for it later.
    */
    .__sinit = (void *)abort,
    ._cleanup_r = &_cleanup_r,
#endif
};

static void raise_r_stub(struct _reent *rptr)
{
    _raise_r(rptr, 0);
}
