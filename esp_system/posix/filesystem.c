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
/** @declaration
****************************************************************************/
struct FS_context
{
    mutex_t lock;
    /// @working directory
    int working_dirfd;
    /// @collection of free ext blocks
    glist_t ext_buf_list;
    /// ext_buf_preallocated
    struct FS_ext prealloc[16];
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
static int FILESYSTEM_openat(int dirfd, char const *pathname, int flags, mode_t mode);
static void *FS_extbuf_alloc(void);
static void FS_extbuf_release(void *ptr);
static void FS_dirfd_link_cleanup(int base_dirfd, int fd);
static void FS_dirfd_cleanup(int fd);

/***************************************************************************/
/** @constructor
****************************************************************************/
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

    if (-1 == FS_context.working_dirfd)
        chdir("/");
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

    // AsFD(fd)->ext can be null
    if (AsFD(fd)->ext)
        FS_extbuf_release(AsFD(fd)->ext);
}

/***************************************************************************/
/** @implements unistd.h
****************************************************************************/
int FILESYSTEM_format(char const *pathmnt, char const *fstype)
{
    int fd = FILESYSTEM_openat(FS_context.working_dirfd, pathmnt, O_DIRECTORY, 0);
    if (-1 == fd)
        return fd;

    if (NULL == AsFD(fd)->fs->format)
        return __set_errno_neg(EPERM);
    else
        return AsFD(fd)->fs->format(AsFD(fd)->ext, fstype);
}

int chdir(char const *pathname)
{
    int fd = FILESYSTEM_openat(FS_context.working_dirfd, pathname, O_DIRECTORY, 0);
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
            struct FS_ext *ext = AsFD(fd)->ext;

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
/** @implements fcntl.h / io.h
****************************************************************************/
int creat(char const *pathname, mode_t mode)
{
    return FILESYSTEM_openat(FS_context.working_dirfd, pathname,
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
        value = AsFD(fd)->flags;

        if (FD_FLAG_NONBLOCK & value)
            retval |= O_NONBLOCK;
        break;
    }

    va_end(vl);
    return retval;
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

    return FILESYSTEM_openat(FS_context.working_dirfd, pathname, flags, mode);
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

    return FILESYSTEM_openat(dirfd, pathname, flags, mode);
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

    int fd = FILESYSTEM_openat(dirfd, pathname, oflag, 0);
    if (-1 == fd)
        return fd;

    /// @acquire ino
    ino_t ino = ((struct FS_ext *)AsFD(fd)->ext)->ino_entry;

    /// get @parent's filesystem it should always dirfd
    struct KERNEL_fd *parent = AsFD(fd)->glist_next;
    struct FS_implement const *fs = parent->fs;
    void *ext = (struct FS_ext *)parent->ext;

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
        return fs->truncate(AsFD(fd)->ext, size);
    else
        return __set_errno_neg(ENOSYS);
}

/***************************************************************************/
/** @implements dirent.h
****************************************************************************/
DIR *opendir(char const *pathname)
{
    int fd = FILESYSTEM_openat(FS_context.working_dirfd, pathname, O_DIRECTORY, 0);

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
            ((struct dirent *)(dirp + 1))->d_ino = (ino_t)-2;
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

    if ((ino_t)-2 == ent->d_ino)
    {
        ent->d_ino ++;
        ent->d_mode = S_IFDIR | S_IRUSR  | S_IRGRP | S_IROTH;
        ent->d_namelen = 1;
        strcpy(ent->d_name, ".");
    }
    else if ((ino_t)-1 == ent->d_ino)
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
    ((struct dirent *)(dirp + 1))->d_ino = (ino_t)-2;

    struct FS_ext *ext = (struct FS_ext *)AsFD(dirp->fd)->ext;
    ext->ino_entry = ext->ino_working = (ino_t)-2;
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
    return FILESYSTEM_openat(dirfd, pathname, O_DIRECTORY | O_CREAT | O_EXCL, mode);
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
static int FILESYSTEM_openat(int dirfd, char const *pathname, int flags, mode_t mode)
{
    if (NULL == FS_root)
        return __set_errno_neg(ENOSYS);

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

            struct FS_ext *ext = FS_extbuf_alloc();
            if (! ext)
            {
                fd = __set_errno_neg(ENOMEM);
                goto FILESYSTEM_openat_exit;
            }
            ext->ino_entry = ext->ino_working = (ino_t)-2;
            ext->flags = flags;

            fd = FS_root->open(ext);

            AsFD(fd)->fs = FS_root;
            /// make @rootdir'parent self circulation link
            AsFD(fd)->glist_next = (void *)fd;
        }
        else
        {
            fd = __set_errno_neg(ENOENT);
            goto FILESYSTEM_openat_exit;
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
        strncpy(name, p1, namelen);
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

            fd = __set_errno_neg(ENOTDIR);
            goto FILESYSTEM_openat_exit;
        }
        parent_fd = fd;

        struct dirent *ent;
        while (NULL != (ent = readdir(dirp)))
        {
            if (namelen == ent->d_namelen && 0 == strncmp(name, ent->d_name, namelen))
                break;
        }

        struct FS_ext *ext = FS_extbuf_alloc();
        if (! ext)
        {
            close(fd);
            KERNEL_mfree(dirp);

            fd = __set_errno_neg(ENOMEM);
            goto FILESYSTEM_openat_exit;
        }
        ext->ino_entry = ext->ino_working = (ino_t)-2;
        ext->data = ((struct FS_ext *)AsFD(fd)->ext)->data;

        struct FS_implement const *fs = (struct FS_implement const *)AsFD(parent_fd)->fs;
        if (! ent)
        {
            ext->flags = flags & (~O_TRUNC);

            /// checking last of pathname and O_CREAT
            if (*p || ! (O_CREAT & flags))
                fd = __set_errno_neg(ENOENT);
            else if (NULL == fs->create)
                fd = __set_errno_neg(EROFS);
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
            goto FILESYSTEM_openat_exit;
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
            fd = __set_errno_neg(ENOTDIR);
        }
        else
        {
            /// @rewinddir
            AsFD(fd)->position = 0;
        }
    }
    else
        FS_dirfd_link_cleanup(dirfd, fd);

FILESYSTEM_openat_exit:
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
    FS_extbuf_release(AsFD(fd)->ext);

    /// @prevent KERNEL_handle_release() @recursive call FILESYSTEM_fd_cleanup()
    AsFD(fd)->fs = NULL;
    KERNEL_handle_release((void *)fd);
}

static void *FS_extbuf_alloc(void)
{
    FILESYSTEM_lock();
    struct FS_ext *ptr = glist_pop(&FS_context.ext_buf_list);

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
