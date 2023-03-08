/****************************************************************************
  This file is part of UltraCore

  Copyright by UltraCreation Co Ltd 2018
-------------------------------------------------------------------------------
    The contents of this file are used with permission, subject to the Mozilla
  Public License Version 1.1(the "License"); you may not use this file except
  in compliance with the License. You may  obtain a copy of the License at
  http://www.mozilla.org/MPL/MPL-1.1.html

    Software distributed under the License is distributed on an "AS IS" basis,
  WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License for
  the specific language governing rights and limitations under the License.
****************************************************************************/
#include <sys/errno.h>
#include "semaphore.h"

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

static_assert(sizeof_member(struct __semaphore, __pad) == sizeof(StaticSemaphore_t), "sizeof(struct __semaphore) != sizeof(StaticSemaphore_t)");

/***************************************************************************
 *  @implements
 ***************************************************************************/
int sem_init(sem_t *sema, int pshared, unsigned int value)
{
    if (pshared)
        return __set_errno_neg(ENOSYS);
    if (SEM_VALUE_MAX < value)
        return __set_errno_neg(EINVAL);

    if (NULL == xSemaphoreCreateCountingStatic(value, 0, (void *)&sema->__pad))
        return __set_errno_neg(ENOMEM);
    else
        return 0;
}

int sem_destroy(sem_t *sema)
{
    vSemaphoreDelete((void *)&sema->__pad);
    return 0;
}

sem_t *sem_open(char const *name, int oflag, ...)
{
    ARG_UNUSED(name, oflag);
    return __set_errno_nullptr(ENOSYS);
}

int sem_close(sem_t *sema)
{
    ARG_UNUSED(sema);
    return __set_errno_neg(ENOSYS);
}

int sem_unlink(char const *name)
{
    ARG_UNUSED(name);
    return __set_errno_neg(ENOSYS);
}

int sem_wait(sem_t *sema)
{
    if (pdTRUE == xSemaphoreTake((void *)&sema->__pad, portMAX_DELAY))
        return 0;
    else
        return __set_errno_neg(EINTR);  // REVIEW: EINTR?
}

int sem_timedwait(sem_t *sema, struct timespec const *spec)
{
    return sem_timedwait_ms(sema, (spec->tv_sec * 1000 + spec->tv_nsec / 1000000) / portTICK_PERIOD_MS);
}

int sem_timedwait_ms(sem_t *sema, unsigned int millisecond)
{
    if (pdTRUE == xSemaphoreTake((void *)&sema->__pad, millisecond / portTICK_PERIOD_MS))
        return 0;
    else
        return __set_errno_neg(ETIMEDOUT);
}

int sem_post(sem_t *sema)
{
    if (pdTRUE == xSemaphoreGive(&sema->__pad))
        return 0;
    else
        return __set_errno_neg(EOVERFLOW);
}

int sem_getvalue(sem_t *sema, int *val)
{
    *val = uxSemaphoreGetCount(&sema->__pad);
    return 0;
}
