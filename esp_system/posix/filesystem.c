#include <rtos/kernel.h>

#include <stdarg.h>
#include <string.h>
#include <limits.h>
#include <sys/errno.h>
#include <sys/stat.h>

/// @implements headers
#include <fcntl.h>
#include <dirent.h>
#include <unistd.h>

/***************************************************************************/
/** @def
****************************************************************************/
#define INO_PARENT_DIR                  ((ino_t)-1)
#define INO_CURRENT_DIR                 ((ino_t)-2)

struct FS_context
{
    mutex_t lock;
    /// @working directory
    int working_dirfd;
    /// @collection of free ext blocks
    glist_t ext_buf_list;
    /// ext_buf_preallocated
    struct fsio_t prealloc[20];
};

/***************************************************************************/
/** @export
****************************************************************************/
// weaked, everything ENOSYS if this is NULL
__attribute__((weak)) struct FS_implement const *FS_root = NULL;

// @overrides context.c weak reference
extern __attribute__((nothrow))
    void FILESYSTEM_fd_cleanup(int fd);

/***************************************************************************/
/** @internal
****************************************************************************/
/// @variable
static struct FS_context FS_context = {0};

/// @function
static int FS_openat(struct _reent *r, int dirfd, char const *pathname, int flags, mode_t mode);
static void *FS_extbuf_alloc(void);
static void FS_extbuf_release(void *ptr);
static void FS_dirfd_link_cleanup(int base_dirfd, int fd);
static void FS_dirfd_cleanup(int fd);

/***************************************************************************/
/** @constructor
****************************************************************************/
void __FILESYSTEM_introduce(void)
{
}

#ifdef __GNUC__
    #pragma GCC diagnostic push
    #pragma GCC diagnostic ignored "-Wmissing-prototypes"
#endif

__attribute__((constructor, nothrow))
static void FILESYSTEM_initialize(void)
{
    mutex_init(&FS_context.lock, MUTEX_FLAG_RECURSIVE);
    glist_initialize(&FS_context.ext_buf_list);
    FS_context.working_dirfd = -1;

    for (unsigned i = 0; i < lengthof(FS_context.prealloc); i ++)
        glist_push_back(&FS_context.ext_buf_list, &FS_context.prealloc[i]);

    FILESYSTEM_init_root();
    FILESYSTEM_startup();
}

#ifdef __GNUC__
    #pragma GCC diagnostic pop
#endif

/***************************************************************************/
/** @implements filesystem.h
****************************************************************************/
__attribute__((weak))
void FILESYSTEM_startup(void)
{
}

__attribute__((weak))
void FILESYSTEM_init_root(void)
{
}

void FILESYSTEM_lock(void)
{
    mutex_lock(&FS_context.lock);
}

void FILESYSTEM_unlock(void)
{
    mutex_unlock(&FS_context.lock);
}

void FILESYSTEM_fd_cleanup(int fd)
{
    FS_dirfd_link_cleanup(-1, fd);

    // AsFD(fd)->fsio can be null
    if (AsFD(fd)->fsio)
        FS_extbuf_release(AsFD(fd)->fsio);
}

/***************************************************************************/
/** @implements unistd.h
****************************************************************************/
int FILESYSTEM_format(char const *pathmnt, char const *fstype)
{
    int fd = FS_openat(NULL, FS_context.working_dirfd, pathmnt, O_DIRECTORY, 0);
    if (-1 == fd)
        return fd;

    if (NULL == AsFD(fd)->fs->format)
        return __set_errno_neg(EPERM);
    else
        return AsFD(fd)->fs->format(AsFD(fd)->fsio, fstype);
}

int chdir(char const *pathname)
{
    int fd = FS_openat(NULL, FS_context.working_dirfd, pathname, O_DIRECTORY, 0);
    if (-1 == fd)
        return fd;
    else
        return fchdir(fd);
}

int fchdir(int fd)
{
    if (! (FD_TAG_DIR & AsFD(fd)->tag))
        return __set_errno_neg(ENOTDIR);

    int old_dirfd = FS_context.working_dirfd;
    FS_context.working_dirfd = fd;

    if (-1 != old_dirfd)
    {
        bool clean_old_dirfd = true;
        struct KERNEL_fd *iter_dirfd = AsFD(fd);

        /// @find if old dirfd in the new dirfd's chain
        while (true)
        {
            if ((int)iter_dirfd == old_dirfd)
            {
                clean_old_dirfd = false;
                break;
            }

            /// @rootdir'parent is self circulation link
            if (iter_dirfd == iter_dirfd->glist_next)
                break;
            else
                iter_dirfd = iter_dirfd->glist_next;
        }

        if (clean_old_dirfd) close(old_dirfd);
    }
    return 0;
}

char *getcwd(char *buf, size_t size)
{
    if (0 == FS_context.working_dirfd)
    {
        /// @ERANGE
        if (size < 2)
            return __set_errno_nullptr(ERANGE);

        buf[0] = '/';
        buf[1] = '\0';
    }
    else
    {
        size_t namelen = 0;
        int fd = FS_context.working_dirfd;
        int parent_fd = (int)AsFD(fd)->glist_next;

        while (0 != fd)
        {
            struct fsio_t *fsio = AsFD(fd)->fsio;

            DIR *dirp = fdopendir(parent_fd);
            seekdir(dirp, (off_t)ext->ino_entry);

            struct dirent *ent = readdir(dirp);

            size_t len = namelen + ent->d_namelen + 1;
            /// @ERANGE
            if (size < (size_t)len)
            {
                KERNEL_mfree(dirp);
                return __set_errno_nullptr(ERANGE);
            }

            // its possiable when root filesystem
            if ('.' != ent->d_name[0])
            {
                memmove(&buf[ent->d_namelen + 1], buf, namelen);    // overlapped mem
                memcpy(&buf[1], ent->d_name, ent->d_namelen);
            }
            else
                len --;

            buf[0] = '/';
            buf[len] = '\0';
            namelen = len;

            KERNEL_mfree(dirp);

            fd = parent_fd;
            parent_fd = (int)AsFD(fd)->glist_next;

            /// @rootdir'parent is self circulation link
            if (parent_fd == fd) break;
        }

        buf[namelen] = '\0';
    }
    return buf;
}

char *getwd(char *buf)
{
    return getcwd(buf, PATH_MAX);
}

char *get_current_dir_name(void)
{
    char *retval = malloc(PATH_MAX);
    return getcwd(retval, PATH_MAX);
}

/***************************************************************************/
/** @implements fcntl.h / fdio.h
****************************************************************************/
int creat(char const *pathname, mode_t mode)
{
    return FS_openat(NULL, FS_context.working_dirfd, pathname,
        O_WRONLY | O_CREAT | O_TRUNC, mode);
}

int fcntl(int fd, int cmd, ...)
{
    if (fd <= 0 || CID_FD != AsFD(fd)->cid)
        return EBADF;

    va_list vl;
    va_start(vl, cmd);

    int retval = 0;
    unsigned int value;

    /**
     *  cmd F_GETFL / F_SETFL
     *      On Linux, this command can change only the O_APPEND, O_ASYNC, O_DIRECT, O_NOATIME, and O_NONBLOCK flags.
     *      On UltraCore, only O_NONBLOCK is supported
     */
    switch (cmd)
    {
    case F_SETFL:
        value = va_arg(vl, unsigned int);

        if (O_NONBLOCK & value)
            AsFD(fd)->flags |= FD_FLAG_NONBLOCK;
        else
            AsFD(fd)->flags &= (uint8_t)~FD_FLAG_NONBLOCK;
        break;

    case F_GETFL:
        if (FD_FLAG_NONBLOCK & AsFD(fd)->flags)
            retval |= O_NONBLOCK;
        break;
    }

    va_end(vl);
    return retval;
}

int _fcntl_r(struct _reent *r, int fd, int cmd, int value)
{
    ARG_UNUSED(r);
    return fcntl(fd, cmd, value);
}

int _open_r(struct _reent *r, char const *pathname, int flags, mode_t mode)
{
    ARG_UNUSED(r);
    return FS_openat(r, FS_context.working_dirfd, pathname, flags, mode);
}

int open(char const *pathname, int flags, ...)
{
    mode_t mode;

    if (O_CREAT & flags)
    {
        va_list vl;
        va_start(vl, flags);

        mode = va_arg(vl, mode_t);
        va_end(vl);
    }
    else
        mode = 0;

    return FS_openat(NULL, FS_context.working_dirfd, pathname, flags, mode);
}

int openat(int dirfd, char const *pathname, int flags, ...)
{
    mode_t mode;

    if (O_CREAT & flags)
    {
        va_list vl;
        va_start(vl, flags);
        mode = va_arg(vl, mode_t);
        va_end(vl);
    }
    else
        mode = 0;

    return FS_openat(NULL, dirfd, pathname, flags, mode);
}

int unlink(char const *pathname)
{
    return unlinkat(FS_context.working_dirfd, pathname, 0);
}

int unlinkat(int dirfd, char const *pathname, int flags)
{
    int oflag;
    if (AT_REMOVEDIR & flags)
        oflag = O_DIRECTORY;
    else
        oflag = 0;

    if (AT_FDCWD & flags)
        oflag |= AT_FDCWD;

    int fd = FS_openat(NULL, dirfd, pathname, oflag, 0);
    if (-1 == fd)
        return fd;

    /// @acquire ino
    ino_t ino = ((struct fsio_t *)AsFD(fd)->fsio)->ino_entry;

    /// get @parent's filesystem it should always dirfd
    struct KERNEL_fd *parent = AsFD(fd)->glist_next;
    struct FS_implement const *fs = parent->fs;
    void *ext = (struct fsio_t *)parent->fsio;

    // release fd's memory
    close(fd);

    if (NULL == fs->unlink)
        return __set_errno_neg(EROFS);
    else
        return fs->unlink(ext, ino);
}

int truncate(const char *path, off_t size)
{
    int fd = open(path, O_WRONLY);
    if (0 > fd)
        return fd;
    else
        return ftruncate(fd, size);
}

int ftruncate(int fd, off_t size)
{
    if (fd <= 0 || CID_FD != AsFD(fd)->cid)
        return __set_errno_neg(EBADF);

    struct FS_implement const *fs = (struct FS_implement const *)AsFD(fd)->fs;
    if (fs->truncate)
        return fs->truncate(AsFD(fd)->fsio, size);
    else
        return __set_errno_neg(ENOSYS);
}

/***************************************************************************/
/** @implements dirent.h
****************************************************************************/
DIR *opendir(char const *pathname)
{
    int fd = FS_openat(NULL, FS_context.working_dirfd, pathname, O_DIRECTORY, 0);

    if (fd < 0)
        return NULL;
    else
        return fdopendir(fd);
}

DIR *fdopendir(int fd)
{
    if (FD_TAG_DIR & AsFD(fd)->tag)
    {
        struct FS_implement const *fs = AsFD(fd)->fs;

        DIR *dirp = KERNEL_mallocz(sizeof(struct DIR) + fs->dirent_size);
        if (dirp)
        {
            dirp->fd = fd;

            AsFD(dirp->fd)->position = 0;
            ((struct dirent *)(dirp + 1))->d_ino = INO_CURRENT_DIR;
        }

        return dirp;
    }
    else
        return __set_errno_nullptr(ENOTDIR);
}

int closedir(DIR *dirp)
{
    if (FS_context.working_dirfd != dirp->fd)
        close(dirp->fd);

    KERNEL_mfree(dirp);
    return 0;
}

struct dirent *readdir(DIR *dirp)
{
    struct FS_implement const *fs = AsFD(dirp->fd)->fs;
    struct dirent *ent = (struct dirent *)(dirp + 1);

    if (INO_CURRENT_DIR == ent->d_ino)
    {
        ent->d_ino ++;
        ent->d_mode = S_IFDIR | S_IRUSR  | S_IRGRP | S_IROTH;
        ent->d_namelen = 1;
        strcpy(ent->d_name, ".");
    }
    else if (INO_PARENT_DIR == ent->d_ino)
    {
        ent->d_ino ++;
        ent->d_mode = S_IFDIR | S_IRUSR  | S_IRGRP | S_IROTH;
        ent->d_namelen = 2;
        strcpy(ent->d_name, "..");
    }
    else if (read(dirp->fd, ent, fs->dirent_size) > 0)
    {
        if (! ent->d_filesystem)
            ent->d_filesystem = fs;
    }
    else
        ent = NULL;

    return ent;
}

off_t telldir(DIR *dirp)
{
    return tell(dirp->fd);
}

void seekdir(DIR *dirp, off_t location)
{
    struct dirent *ent = (struct dirent *)(dirp + 1);
    ent->d_ino = (ino_t)location;

    if ((off_t)(-1) == location || (off_t)(-2) == location)
        AsFD(dirp->fd)->position = 0;
    else
        AsFD(dirp->fd)->position = (uintptr_t)location;
}

void rewinddir(DIR *dirp)
{
    AsFD(dirp->fd)->position = 0;
    ((struct dirent *)(dirp + 1))->d_ino = INO_CURRENT_DIR;

    struct fsio_t *fsio = (struct fsio_t *)AsFD(dirp->fd)->fsio;
    ext->ino_entry = ext->ino_working = INO_CURRENT_DIR;
}

int dirfd(DIR *dir)
{
    return dir->fd;
}

int alphasort(struct dirent const **a, struct dirent const **b)
{
    return strcoll((*a)->d_name, (*b)->d_name);
}

int scandir(char const *pathname,
    struct dirent ***entrylist,
    int (* filter)(struct dirent const *),
    int (* compar)(struct dirent const **, struct dirent const **))
{
    ARG_UNUSED(pathname, entrylist, filter, compar);
    return 0;
}

/***************************************************************************/
/** @implements stat.h
****************************************************************************/
int stat(char const *restrict pathname, struct stat *restrict st)
{
    int fd = open(pathname, O_RDONLY);

    int retval = fstat(fd, st);
    close(fd);
    return retval;
}

int _fstat_r(struct _reent *r, int fd, struct stat *st)
{
    if (STDIN_FILENO == fd || STDOUT_FILENO == fd || STDERR_FILENO == fd)
    {
        memset(st, 0, sizeof(*st));

        st->st_mode = S_IFCHR;
        return 0;
    }

    if (CID_FD != AsFD(fd)->cid)
        return __set_errno_r_neg(r, EBADF);

    if (FD_TAG_BLOCK & AsFD(fd)->tag)
        st->st_mode |= S_IFBLK;
    if (FD_TAG_CHAR & AsFD(fd)->tag)
        st->st_mode |= S_IFCHR;
    if (FD_TAG_DIR & AsFD(fd)->tag)
        st->st_mode |= S_IFDIR;
    if (FD_TAG_FIFO & AsFD(fd)->tag)
        st->st_mode |= S_IFIFO;
    if (FD_TAG_LINK & AsFD(fd)->tag)
        st->st_mode |= S_IFLNK;
    if (FD_TAG_REG & AsFD(fd)->tag)
        st->st_mode |= S_IFREG;
    if (FD_TAG_SOCKET & AsFD(fd)->tag)
        st->st_mode |= S_IFSOCK;

    // TODO: complete fstat() struct stat other fields
    return 0;
}

int fstat(int fd, struct stat *st)
{
    return _fstat_r(__getreent(), fd, st);
}

int mkdir(char const *pathname, mode_t mode)
{
    return mkdirat(FS_context.working_dirfd, pathname, mode);
}

int mkdirat(int dirfd, char const *pathname, mode_t mode)
{
    return FS_openat(NULL, dirfd, pathname, O_DIRECTORY | O_CREAT | O_EXCL, mode);
}

int rmdirat(int dirfd, char const *pathname)
{
    return unlinkat(dirfd, pathname, AT_REMOVEDIR);
}

int rmdir(char const *pathname)
{
    return rmdirat(FS_context.working_dirfd, pathname);
}

/***************************************************************************/
/** @private
****************************************************************************/
static int FS_openat(struct _reent *r, int dirfd, char const *pathname, int flags, mode_t mode)
{
    if (NULL == r)
        r = __getreent();
    if (NULL == FS_root)
        return __set_errno_r_neg(r, ENOENT);

    char *name = KERNEL_malloc(NAME_MAX);
    char const *p = pathname;

    int fd, parent_fd = 0;

    // ignore dirfd?
    if (AT_FDCWD & flags)
        dirfd = FS_context.working_dirfd;

    if (-1 == dirfd || '/' == p[0])
    {
        if ('/' == p[0] || '\0' == p[0] || '.' == p[0])
        {
            if (*p) p ++;

            struct fsio_t *fsio = FS_extbuf_alloc();
            if (! ext)
            {
                fd = __set_errno_r_neg(r, ENOMEM);
                goto FS_openat_exit;
            }
            ext->ino_entry = ext->ino_working = INO_CURRENT_DIR;
            ext->flags = flags;

            fd = FS_root->open(ext);

            AsFD(fd)->fs = FS_root;
            /// make @rootdir'parent self circulation link
            AsFD(fd)->glist_next = (void *)fd;
        }
        else
        {
            fd = __set_errno_r_neg(r, ENOENT);
            goto FS_openat_exit;
        }
    }
    else
    {
        fd = dirfd;
        parent_fd = (int)AsFD(fd)->glist_next;
    }

    while (true)
    {
        if (! (*p)) break;

        char const *p1 = p;
        while (*p && *p != '/') p ++;

        size_t namelen = (size_t)(p - p1);
        strlcpy(name, p1, namelen);
        name[namelen] = '\0';

        if (*p == '/')
            p ++;

        if (0 == strcmp("..", name))
        {
            /// parent_fd is always exists due to @rootdir'parent is self circulation link
            fd = parent_fd;
            parent_fd = (int)AsFD(fd)->glist_next;

            fchdir(fd);
            continue;
        }
        else if (0 == strcmp(".", name))
            continue;

        DIR *dirp = fdopendir(fd);
        if (! dirp)
        {
            close(fd);

            fd = __set_errno_r_neg(r, ENOTDIR);
            goto FS_openat_exit;
        }
        parent_fd = fd;

        struct dirent *ent;
        while (NULL != (ent = readdir(dirp)))
        {
            if (namelen == ent->d_namelen && 0 == strncmp(name, ent->d_name, namelen))
                break;
        }

        struct fsio_t *fsio = FS_extbuf_alloc();
        if (! ext)
        {
            close(fd);
            KERNEL_mfree(dirp);

            fd = __set_errno_r_neg(r, ENOMEM);
            goto FS_openat_exit;
        }
        ext->ino_entry = ext->ino_working = INO_CURRENT_DIR;
        ext->data = ((struct fsio_t *)AsFD(fd)->fsio)->data;

        struct FS_implement const *fs = (struct FS_implement const *)AsFD(parent_fd)->fs;
        if (! ent)
        {
            ext->flags = flags & (~O_TRUNC);

            /// checking last of pathname and O_CREAT
            if (*p || ! (O_CREAT & flags))
                fd = __set_errno_r_neg(r, ENOENT);
            else if (NULL == fs->create)
                fd = __set_errno_r_neg(r, EROFS);
            else
                fd = fs->create(ext, name, mode);
        }
        else
        {
            ext->ino_entry = ext->ino_working = ent->d_ino;
            ext->size = ent->d_size;
            ext->flags = flags & (~O_CREAT);

            fd = fs->open(ext);
            // fd's filesystem should be ent->d_filesystem
            fs = ent->d_filesystem;
        }
        KERNEL_mfree(dirp);

        if (fd < 0)
        {
            if (parent_fd != dirfd)
                close(parent_fd);
            goto FS_openat_exit;
        }
        else
        {
            AsFD(fd)->glist_next = (void *)parent_fd;

            AsFD(fd)->fs = fs;
            AsFD(fd)->implement = fs->fsio;
        }
    }

    if (FD_TAG_DIR & AsFD(fd)->tag)
    {
        if (! (O_DIRECTORY & flags))
        {
            if (fd != dirfd)
                close(fd);
            fd = __set_errno_r_neg(r, ENOTDIR);
        }
        else
        {
            /// @rewinddir
            AsFD(fd)->position = 0;
        }
    }
    else
        FS_dirfd_link_cleanup(dirfd, fd);

FS_openat_exit:
    free(name);
    return fd;
}

static void FS_dirfd_link_cleanup(int base_dirfd, int fd)
{
    if (-1 == base_dirfd)
        base_dirfd = FS_context.working_dirfd;

    struct KERNEL_fd *dirfd_iter = AsFD(fd)->glist_next;

    /// opened @rootdir'parent is a self circulation link
    while (fd != (int)dirfd_iter && base_dirfd != (int)dirfd_iter)
    {
        fd = (int)dirfd_iter;
        dirfd_iter = dirfd_iter->glist_next;

        FS_dirfd_cleanup(fd);
    }
}

static void FS_dirfd_cleanup(int fd)
{
    FS_extbuf_release(AsFD(fd)->fsio);

    /// @prevent KERNEL_handle_release() @recursive call FILESYSTEM_fd_cleanup()
    AsFD(fd)->fs = NULL;
    KERNEL_handle_release((void *)fd);
}

static void *FS_extbuf_alloc(void)
{
    FILESYSTEM_lock();
    struct fsio_t *ptr = glist_pop(&FS_context.ext_buf_list);

    if (! ptr)
    {
        ptr = malloc(sizeof(FS_context.prealloc) / 2);

        for (size_t i = 1; i < lengthof(FS_context.prealloc) / 2; i ++)
            glist_push_back(&FS_context.ext_buf_list, &ptr[i]);
    }
    FILESYSTEM_unlock();

    memset(ptr, 0, sizeof(*ptr));
    return ptr;
}

static void FS_extbuf_release(void *ptr)
{
    if (0 == __get_IPSR())
    {
        FILESYSTEM_lock();
        glist_push_back(&FS_context.ext_buf_list, ptr);
        FILESYSTEM_unlock();
    }
    else
        glist_push_back(&FS_context.ext_buf_list, ptr);
}
