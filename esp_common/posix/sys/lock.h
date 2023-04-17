#ifndef __SYS_LOCK_H
#define __SYS_LOCK_H                   1

#include_next <sys/lock.h>
#include "mutex.h"

struct __lock
{
    mutex_t mutex;
};

#endif
