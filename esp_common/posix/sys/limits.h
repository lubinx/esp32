/****************************************************************************
  This file is part of UltraCore
  Copyright by UltraCreation Co Ltd 2018

  Reference from IEEE Standard for Information Technology -
    Portable Operating System Interface (POSIX®) Issue 7 2018 Edition
  The original  Standard can be obtained online at
    http://www.unix.org/online.html.
-------------------------------------------------------------------------------
    The contents of this file are used with permission, subject to the Mozilla
  Public License Version 1.1 (the "License"); you may not use this file except
  in compliance with the License. You may  obtain a copy of the License at
  http://www.mozilla.org/MPL/MPL-1.1.html

    Software distributed under the License is distributed on an "AS IS" basis,
  WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License for
  the specific language governing rights and limitations under the License.
****************************************************************************/
#ifndef __SYS_LIMITS_H
#define __SYS_LIMITS_H

/* Pathname Variable Values */
    /**
     *      Minimum number of bits needed to represent, as a signed integer value, the maximum size
     *  of a regular file allowed in the specified directory.
     *  .Minimum Acceptable Value: 32
     */
    #define FILESIZEBITS                32
    /**
     *      Maximum number of links to a single file.
     *  .Minimum Acceptable Value: {_POSIX_LINK_MAX}
     */
    #define LINK_MAX                    512

    /**
     *      Maximum number of bytes in a terminal canonical input line. Minimum Acceptable Value: {_POSIX_MAX_CANON}
     */
    #define MAX_CANON                   1024

    /**      Minimum number of bytes for which space is available in a terminal input queue; therefore,
     *  the maximum number of bytes a conforming application may require to be typed as input before reading them.
     *  .Minimum Acceptable Value: {_POSIX_MAX_INPUT}
     */
    #define MAX_INPUT                   0

    /**
     *      Maximum number of bytes in a filename (not including the terminating null of a filename string).
     *  .Minimum Acceptable Value: {_POSIX_NAME_MAX}
     *  .XSI Minimum Acceptable Value: {_XOPEN_NAME_MAX}
     */
    #define NAME_MAX                    64
    /**
     *      Maximum number of bytes the implementation will store as a pathname in a user-supplied
     *  buffer of unspecified size, including the terminating null character. Minimum number the implementation
     *  will accept as the maximum number of bytes in a pathname.
     *  .Minimum Acceptable Value: {_POSIX_PATH_MAX}
     *  .XSI Minimum Acceptable Value: {_XOPEN_PATH_MAX}
     */
    #define PATH_MAX                    255

    /**     Maximum number of bytes that is guaranteed to be atomic when writing to a pipe.
     *  .Minimum Acceptable Value: {_POSIX_PIPE_BUF}
     */
    #define PIPE_BUF                    2048
    /**
     *      Maximum number of bytes in a symbolic link.
     *  .Minimum Acceptable Value: {_POSIX_SYMLINK_MAX}
     */
    #define SYMLINK_MAX                 128

// include next
#include <limits.h>
#endif
