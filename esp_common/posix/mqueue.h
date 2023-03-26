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
#ifndef __MQUEUE_H
#define __MQUEUE_H                      1

#include <features.h>
#include <fcntl.h>
#include <sys/types.h>

__BEGIN_DECLS

typedef int                         mqd_t;

struct mq_attr
{
    // Message queue flags.
    uint32_t mq_flags;
    // Maximum number of messages.
    uint32_t mq_maxmsg;
    // Maximum message size.
    uint32_t mq_msgsize;
    // Number of messages currently queued.
    uint32_t mq_curmsgs;
};

extern  __attribute__((nothrow, nonnull(1)))
    mqd_t mq_open(char const *name, int flags, /* optional mode_t, struct mq_attr * */...);
extern  __attribute__((nothrow))
    int mq_close(mqd_t mqd);
extern  __attribute__((nothrow, nonnull(1)))
    int mq_unlink(char const *name);

extern  __attribute__((nothrow, nonnull(2)))
    int mq_getattr(mqd_t mqd, struct mq_attr *attr);
extern  __attribute__((nothrow, nonnull(2)))
    int mq_setattr(mqd_t mqd, struct mq_attr const *restrict attr, struct mq_attr *restrict oattr);

extern  __attribute__((nothrow))
    int mq_notify(mqd_t mqd, struct sigevent const *notification);

extern  __attribute__((nothrow, nonnull(2)))
    ssize_t mq_receive(mqd_t mqd, char *ptr, size_t len, unsigned int *prio);
extern  __attribute__((nothrow, nonnull(2, 5)))
    ssize_t mq_timedreceive(mqd_t mqd, char *restrict ptr, size_t len, unsigned int *restrict prio,
        struct timespec const *restrict abs_ts);

extern  __attribute__((nothrow, nonnull(2)))
    int mq_send(mqd_t mqd, char const *ptr, size_t len, unsigned int prio);
extern  __attribute__((nothrow, nonnull(2, 5)))
    int mq_timedsend(mqd_t mqd, char const *ptr, size_t len, unsigned int prio,
        struct timespec const *abs_ts);

__END_DECLS
#endif
