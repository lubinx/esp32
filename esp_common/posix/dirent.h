/****************************************************************************
  This file is part of UltraCore

  Copyright by UltraCreation Co Ltd 2018
-------------------------------------------------------------------------------
    The contents of this file are used with permission, subject to the Mozilla
  Public License Version 1.1 (the "License"); you may not use this file except
  in compliance with the License. You may  obtain a copy of the License at
  http://www.mozilla.org/MPL/MPL-1.1.html

    Software distributed under the License is distributed on an "AS IS" basis,
  WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License for
  the specific language governing rights and limitations under the License.
****************************************************************************/
#ifndef __DIRENT_H
#define __DIRENT_H                      1

#include <features.h>
#include <stdio.h>
#include <sys/types.h>

__BEGIN_DECLS

    struct dirent
    {
        /// dirent's filesystem: reading a @symlink can changing the filesystem
        ///     otherwise it will keep the owner directory's filesystem see @readdir()
        void const *d_filesystem;
        /// see @stat.h st_mode
        mode_t      d_mode;
        /// file serial numbers:
        ///     -2 = "."
        ///     -1 = ".."
        ino_t       d_ino;
        /// file size
        size_t      d_size;
        /// file times
        time_t      d_creation_ts;
        time_t      d_modificaion_ts;
        /// d_namelen
        uint16_t    d_namelen;
        /// @dynamic length (may exceed 6) for real size decide by d_namelen or strlen(d_name)
        char        d_name[6];
    };
    /// get dirent size by length of name
    #define DIRENT_SIZE(NAMELEN)        (offsetof(struct dirent, d_name) + NAMELEN)

    struct DIR
    {
        int fd;
        /// @follows struct dirent with filesystem @designated size
        // struct dirent ent[];
    };
    typedef struct DIR              DIR;

    /**
     *  The opendir() function opens a directory stream corresponding to the directory name, and
     *      returns a pointer to the directory stream.  The stream is positioned at the first entry
     *      in the directory.
     */
extern __attribute__((nonnull, nothrow))
    DIR *opendir(char const *pathname);

    /**
     *  The fdopendir() function is like opendir() but returns a directory stream for the directory
     *      referred to by the open file descriptor fd.
     *  After a successful call to fdopendir(), fd is used internally by the implementation,
     *      and @should @not otherwise be @used by the @application.
     */
extern __attribute__((nothrow))
    DIR *fdopendir(int fd);

    /**
     *  The closedir() function closes the directory stream associated with dirp. A successful call
     *      to closedir() also closes the underlying file descriptor associated with dirp.
     */
extern __attribute__((nonnull, nothrow))
    int closedir(DIR *dirp);

    /**
     *  The readdir() function returns a pointer to a dirent structure representing the next directory
     *      entry in the directory stream pointed to by dirp. It returns NULL on reaching the end of
     *      the directory stream or if an error occurred.
     */
extern __attribute__((nonnull, nothrow))
    struct dirent *readdir(DIR *dirp);

    /**
     *  The telldir() function returns the current location associated with the directory stream dirp.
     */
extern __attribute__((nonnull, nothrow))
    off_t telldir(DIR *dirp);

    /**
     *  The seekdir() function sets the location in the directory stream from which the next readdir()
     *      call will start.  The location argument should be a value returned by a previous call to
     *      telldir().
     */
extern __attribute__((nonnull, nothrow))
    void seekdir(DIR *dirp, off_t location);

extern __attribute__((nonnull, nothrow))
    void rewinddir(DIR *dirp);

extern __attribute__((nonnull, nothrow))
    int dirfd(DIR *dir);

extern __attribute__((nonnull, nothrow))
    int alphasort(struct dirent const **a, struct dirent const **b);

extern __attribute__((nothrow))
    int scandir(char const *pathname, struct dirent ***entrylist,
        int (* filter)(struct dirent const *),
        int (* compar)(struct dirent const **, struct dirent const **));

__END_DECLS
#endif
