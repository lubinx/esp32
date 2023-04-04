#ifndef __SYS_MOUNT_H
#define __SYS_MOUNT_H                   1

#include <features.h>

    /** The umount2() flag to force unmounting. */
    #define MNT_FORCE 1
    /** The umount2() flag to lazy unmount. */
    #define MNT_DETACH 2
    /** The umount2() flag to mark a mount point as expired. */
    #define MNT_EXPIRE 4
    /** The umount2() flag to not dereference the mount point path if it's a symbolic link. */
    #define UMOUNT_NOFOLLOW 8

__BEGIN_DECLS

extern __attribute__((nothrow))
    int mount(char const *mount_point, char const *name, char const *fs_type, unsigned long flags, void const *data);

extern __attribute__((nothrow))
    int umount(char const *target);

__END_DECLS
#endif
