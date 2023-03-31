/* NOTE: this is not full porting ultracore for esp32
    not all function is implemented
*/

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
#ifndef __FS_FILESYSTEM_H
#define __FS_FILESYSTEM_H               1

#include <features.h>
#include <stdint.h>
#include <sys/types.h>

__BEGIN_DECLS

    /**
     *  FILESYSTEM_startup()
     *      this function will be execute before main() for loading fstab
     *      **weak** for application to override it
     */
extern __attribute__((nothrow))
    void FILESYSTEM_startup(void);

    /**
     *  fd->ext of filesystem
     */
    struct FS_ext
    {
        /// the data parameter pass by mount()
        void *data;
        /**
         *  multi purpuse flags parameters
         *      .parameter passed by mount() when dirfd link to another filesystem
         *      .use to call open() / create() / unlink()
         */
        int flags;

        /// file or directory's entry ino fd was hold
        ino_t ino_entry;
        /// file or directory's current ino fd was hold
        ino_t ino_working;
        /// file or directory's size
        size_t size;
    };

    struct FS_implement
    {
        char *name;

        uint16_t dirent_size;
        uint16_t __pad;
        struct FD_implement const *fsio;

        int (* format)  (struct FS_ext *ext, char const *fstype);
        /// filesystem open file/directory
        int (* open)    (struct FS_ext *ext);
        /// filesystem create file/directory
        int (* create)  (struct FS_ext *ext, char const *name, mode_t mode);
        /// filesystem remove file/directory
        int (* truncate)(struct FS_ext *ext, off_t size);
        /// filesystem remove file/directory
        int (* unlink)  (struct FS_ext *ext, ino_t ino);
    };
    struct FD_implement;

    /**
     *  FILESYSTEM_lock() / FILESYSTEM_unlock()
     */
extern __attribute((nothrow))
    void FILESYSTEM_lock(void);
extern __attribute((nothrow))
    void FILESYSTEM_unlock(void);

/***************************************************************************/
/** @ROOT implement
****************************************************************************/
    /**
     *  FS_root
     *      point to root implement of filesystem
     */
extern
    struct FS_implement const *FS_root;

    /**
     *  FILESYSTEM_init_root()
     */
extern __attribute__((nothrow))
    void FILESYSTEM_init_root(void);

    /**
     *  ROOT_mount()
     *      mount filesystem into '/mnt'
     */
extern __attribute__((nonnull, nothrow))
    int ROOT_mount(char const *name, struct FS_implement const *fs, void *data);

    /**
     *  ROOT_unmount()
     *      unmount '/mnt/name'
     *  @returns
     *      data ROOT_mount was provided
     */
extern __attribute__((nonnull, nothrow))
    void *ROOT_unmount(char const *name);

__END_DECLS
#endif
