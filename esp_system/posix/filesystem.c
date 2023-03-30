#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <dirent.h>
#include <reent.h>

#include <sys/errno.h>
#include <sys/fcntl.h>
#include <sys/ioctl.h>

/***************************************************************************/
/** @implements
****************************************************************************/
int _fcntl_r(struct _reent *r, int fd, int cmd, int arg)
{
    ARG_UNUSED(fd, cmd, arg);
    return __set_errno_r_neg(r, ENOSYS);
}

int _fstat_r(struct _reent *r, int fd, struct stat *st)
{
    if (STDIN_FILENO == fd || STDOUT_FILENO == fd || STDERR_FILENO == fd)
    {
        memset(st, 0, sizeof(*st));

        st->st_mode = S_IFCHR;
        return 0;
    }
    else
        return __set_errno_r_neg(r, ENOSYS);
}

int _open_r(struct _reent *r, char const *path, int flags, int mode)
{
    ARG_UNUSED(path, flags, mode);
    return __set_errno_r_neg(r, ENOSYS);
}

int _stat_r(struct _reent *r, char const *path, struct stat *st)
{
    int fd = _open_r(r, path, 0, 0);

    if (0 > fd)
        return fd;

    int retval = _fstat_r(r, fd, st);
    _close_r(r, fd);

    return retval;
}
