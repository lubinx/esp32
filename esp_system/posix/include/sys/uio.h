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
#ifndef __SYS_UIO_H
#define __SYS_UIO_H                     1

#include <features.h>
#include <sys/types.h>

__BEGIN_DECLS

    struct iovec
    {
        void *oiv_base;
        size_t iov_len;
    };

    #ifndef IOV_MAX
        #define IOV_MAX                 1024
    #endif

    /**
     *  readv()
     *      read a vector
     *  @DESCRIPTION
     *      The readv() function shall be equivalent to read(), except as described below. The readv()
     *          function shall place the input data into the iovcnt buffers specified by the members of the iov
     *          array: iov[0], iov[1], . . ., iov[iovcnt−1]. The iovcnt argument is valid if greater than 0 and less than
     *          or equal to {IOV_MAX}.
     *      Each iovec entry specifies the base address and length of an area in memory where data should
     *          be placed. The readv() function shall always fill an area completely before proceeding to the next.
     *      Upon successful completion, readv() shall mark for update the last data access timestamp of the file.
     *  @RETURN VALUE
     *      Refer to read().
     *  @ERRORS
     *      Refer to read().
     *      In addition, the readv() function shall fail if:
     *          EINVAL: The sum of the iov_len values in the iov array overflowed an ssize_t.
     *                  The iovcnt argument was less than or equal to 0, or greater than {IOV_MAX}.
     */
extern __attribute__((nonnull, nothrow))
    ssize_t readv(int fd, struct iovec const *iov, int iovcnt);

    /**
     *  writev()
     *      write a vector
     *  @DESCRIPTION
     *      The writev() function shall be equivalent to write(), except as described below. The writev()
     *          function shall gather output data from the iovcnt buffers specified by the members of the iov
     *          array: iov[0], iov[1], . . ., iov[iovcnt−1]. The iovcnt argument is valid if greater than 0 and less than
     *          or equal to {IOV_MAX}, as defined in <limits.h>.
     *      Each iovec entry specifies the base address and length of an area in memory from which data
     *          should be written. The writev() function shall always write a complete area before proceeding to
     *          the next.
     *      If fildes refers to a regular file and all of the iov_len members in the array pointed to by iov are 0,
     *          writev() shall return 0 and have no other effect. For other file types, the behavior is unspecified.
     *      If the sum of the iov_len values is greater than {SSIZE_MAX}, the operation shall fail and no data
     *          shall be transferred.
     *  @RETURN VALUE
     *      Upon successful completion, writev() shall return the number of bytes actually written.
     *      Otherwise, it shall return a value of −1, the file-pointer shall remain unchanged, and errno shall
     *          be set to indicate an error.
     *  @ERRORS
     *      Refer to write().
     *      In addition, the writev() function shall fail if:
     *          EINVAL: The sum of the iov_len values in the iov array would overflow an ssize_t.
     *                  The iovcnt argument was less than or equal to 0, or greater than {IOV_MAX}.
     */
extern __attribute__((nonnull, nothrow))
    ssize_t writev(int fd, struct iovec const *iov, int iovcnt);

__END_DECLS
#endif
