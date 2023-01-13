/*
    libc retarget when bootloader
        at this time, nothing is need to implemented
*/
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>

#pragma GCC diagnostic ignored "-Wattribute-alias"

static int syscall_not_implemented(struct _reent *r, ...)
{
    r->_errno = ENOSYS;
    return -1;
}

int _getpid_r(struct _reent *r)
    __attribute__((alias("syscall_not_implemented")));

int _close_r(struct _reent *r, int fd)
    __attribute__((weak, alias("syscall_not_implemented")));

int _fstat_r (struct _reent *r, int fd, struct stat *st)
    __attribute__((weak, alias("syscall_not_implemented")));

off_t _lseek_r(struct _reent *r, int fd, off_t offset, int origin)
    __attribute__((weak, alias("syscall_not_implemented")));

ssize_t _read_r(struct _reent *r, int fd, void *buf, size_t bufsize)
    __attribute__((weak, alias("syscall_not_implemented")));

int _kill_r(struct _reent *r, int pid, int sig)
    __attribute__((alias("syscall_not_implemented")));

ssize_t _write_r(struct _reent *r, int fd, void const *buf, size_t count)
    __attribute__((weak, alias("syscall_not_implemented")));
