/*
 * SPDX-FileCopyrightText: 2015-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdlib.h>
#include <errno.h>
#include <stdbool.h>
#include <string.h>

#include <sys/stat.h>
#include <sys/reent.h>

#include "esp_rom_caps.h"
#include "esp_newlib.h"

#include "soc/soc_caps.h"

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

/****************************************************************************
 *  imports
*****************************************************************************/
extern __attribute__((nothrow))
    int _printf_float(struct _reent *rptr, void *pdata,
        FILE * fp, int (*pfunc) (struct _reent *, FILE *, const char *, size_t len), va_list * ap);

extern __attribute__((nothrow))
    int _scanf_float(struct _reent *rptr, void *pdata, FILE *fp, va_list *ap);

extern void _cleanup_r(struct _reent* r);

/****************************************************************************
 *  local
*****************************************************************************/
static void raise_r_stub(struct _reent *rptr);

static int syscall_not_implemented(struct _reent *r, ...);
static int syscall_not_implemented_aborts(void);

/****************************************************************************
 *  exports
*****************************************************************************/
void esp_newlib_init(void)
{
    extern void __ubsan_include(void);
    __ubsan_include();

    static const struct syscall_stub_table s_stub_table =
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
            /* TODO IDF-2570 : mark that this assert failed in ROM, to avoid confusion between IDF & ROM
            assertion failures (as function names & source file names will be similar)
            */
            .__assert_func = __assert_func,

            /* We don't expect either ROM code or IDF to ever call __sinit, so it's implemented as abort() for now.

            esp_reent_init() does this job inside IDF.

            Kept in the syscall table in case we find a need for it later.
            */
            .__sinit = (void *)abort,
            ._cleanup_r = &_cleanup_r,
        #endif
    };

    #if CONFIG_IDF_TARGET_ESP32
        syscall_table_ptr_pro = syscall_table_ptr_app = &s_stub_table;
    #elif CONFIG_IDF_TARGET_ESP32S2
        syscall_table_ptr_pro = &s_stub_table;
    #elif CONFIG_IDF_TARGET_ESP32S3 || CONFIG_IDF_TARGET_ESP32C3 || CONFIG_IDF_TARGET_ESP32H4 \
        || CONFIG_IDF_TARGET_ESP32C2 || CONFIG_IDF_TARGET_ESP32C6
        syscall_table_ptr = (void *)&s_stub_table;
    #endif

    static struct _reent s_reent;
    _GLOBAL_REENT = &s_reent;

    static char *env[1] = {0};
    char **environ = env;

    extern void esp_newlib_locks_init(void);
    esp_newlib_locks_init();

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

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wattribute-alias"

int _isatty_r(struct _reent *r, int fd)
    __attribute__((weak, alias("syscall_not_implemented")));

int _open_r(struct _reent *r, const char *path, int flags, int mode)
    __attribute__((weak, alias("syscall_not_implemented")));

int _close_r(struct _reent *r, int fd)
    __attribute__((weak, alias("syscall_not_implemented")));

off_t _lseek_r(struct _reent *r, int fd, off_t size, int mode)
    __attribute__((weak, alias("syscall_not_implemented")));

ssize_t _read_r(struct _reent *r, int fd, void * dst, size_t size)
    __attribute__((weak, alias("syscall_not_implemented")));

ssize_t _write_r(struct _reent *r, int fd, const void * data, size_t size)
    __attribute__((weak, alias("syscall_not_implemented")));

int _fstat_r(struct _reent *r, int fd, struct stat *st)
    __attribute__((weak, alias("syscall_not_implemented")));

int _fcntl_r(struct _reent *r, int fd, int cmd, int arg)
    __attribute__((weak, alias("syscall_not_implemented")));
int _stat_r(struct _reent *r, const char *path, struct stat * st)
    __attribute__((weak, alias("syscall_not_implemented")));

int _link_r(struct _reent *r, const char* n1, const char* n2)
    __attribute__((weak, alias("syscall_not_implemented")));
int _unlink_r(struct _reent *r, const char *path)
    __attribute__((weak, alias("syscall_not_implemented")));

int _rename_r(struct _reent *r, const char *src, const char *dst)
    __attribute__((weak, alias("syscall_not_implemented")));

int _getpid_r(struct _reent *r)
    __attribute__((weak, alias("syscall_not_implemented")));
int _kill_r(struct _reent *r, int pid, int sig)
    __attribute__((weak, alias("syscall_not_implemented")));

/* These functions are not expected to be overridden */
int _system_r(struct _reent *r, const char *str)
    __attribute__((alias("syscall_not_implemented")));

#pragma GCC diagnostic pop

/****************************************************************************
 *  local
*****************************************************************************/
static void raise_r_stub(struct _reent *rptr)
{
    _raise_r(rptr, 0);
}

static int syscall_not_implemented(struct _reent *r, ...)
{
    __errno_r(r) = ENOSYS;
    return -1;
}

static int syscall_not_implemented_aborts(void)
{
    abort();
}
