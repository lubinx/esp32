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
#include <rtos/kernel.h>
#include <semaphore.h>
#include <string.h>
#include <unistd.h>

#include <mqueue.h>

/***************************************************************************/
/** @def
****************************************************************************/
struct MQ_msg
{
    struct MQ_msg *glist_next;
    unsigned int prio;

    uint8_t payload[sizeof(uintptr_t)];
};

struct MQ_list
{
    sem_t sema;
    glist_t list;
};

struct MQ_ext
{
    struct MQ_list queued;
    struct MQ_list freed;
    unsigned int lowest_prio_queued;

    uint16_t msg_max;
    uint16_t msg_size;

    uint8_t __msg_start[sizeof(uint32_t)];
};

/***************************************************************************/
/** @internal
****************************************************************************/
static int SVC_mq_find(char const *name, bool extract);

static struct MQ_msg *SVC_mqueue_get(struct MQ_list *queue, struct MQ_ext *ext);
static void SVC_mqueue_post(struct MQ_list *queue, struct MQ_msg *msg, struct MQ_ext *ext);
static void SVC_mqueue_release(struct MQ_list *queue, struct MQ_msg *msg);

static int mqd_close(int mqd);
static ssize_t mqd_read(int mqd, void *buf, size_t bufsize);
static ssize_t mqd_write(int mqd, void const *buf, size_t count);

/// @const
static struct FD_implement const mqdio =
{
    .read   = mqd_read,
    .write  = mqd_write,
    .close  = mqd_close,
};

/// @var
static spinlock_t MQ_atomic;
static glist_t mq_named_list = GLIST_INITIALIZER(mq_named_list);

/***************************************************************************/
/** @implements ultracore.h
****************************************************************************/
int mqueue_create(char const *name, uint16_t msg_size, uint16_t msg_count)
{
    if (msg_size > MQUEUE_MAX_MSG_SIZE || msg_count > MQUEUE_MAX_MSG)
        return __set_errno_neg(EINVAL);

    size_t __msg_size = msg_size + offsetof(struct MQ_msg, payload);

    struct MQ_ext *ext = KERNEL_mallocz(offsetof(struct MQ_ext, __msg_start) + __msg_size * msg_count);
    if (NULL == ext)
        return __set_errno_neg(ENOMEM);

    spin_lock(&MQ_atomic);

    int mqd = SVC_mq_find(name, false);
    if (-1 != mqd)
    {
        spin_unlock(&MQ_atomic);
        return -1;
    }

    mqd = KERNEL_createfd(FD_TAG_MQD, &mqdio, ext);
    if (-1 != mqd)
    {
        AsMqd(mqd)->name = name;

        // store attr
        ext->msg_max = msg_count;
        ext->msg_size = msg_size;
        ext->lowest_prio_queued = 0;

        // none queued at beginning
        sem_init(&ext->queued.sema, 0, 0);
        glist_initialize(&ext->queued.list);

        // all freed at beginning
        sem_init(&ext->freed.sema, 0, msg_count);
        glist_initialize(&ext->freed.list);

        AsMqd(mqd)->read_rdy = &ext->queued.sema;
        AsMqd(mqd)->write_rdy = &ext->freed.sema;

        // put all messages into freed
        for (size_t i = 0 ; i < msg_count; i ++)
            glist_push_front(&ext->freed.list, ext->__msg_start + i * __msg_size);

        glist_push_back(&mq_named_list, AsMqd(mqd));
    }
    else
        KERNEL_mfree(ext);

    spin_unlock(&MQ_atomic);
    return mqd;
}

int mqueue_destroy(int mqd)
{
    if (CID_FD == AsMqd(mqd)->cid && FD_TAG_MQD == (FD_TAG_MQD & AsMqd(mqd)->tag))
        return close(mqd);
    else
        return EBADF;
}

int mqueue_flush(int mqd)
{
    if (CID_FD == AsMqd(mqd)->cid && FD_TAG_MQD == (FD_TAG_MQD & AsMqd(mqd)->tag))
    {
        struct MQ_ext *ext = AsMqd(mqd)->ext;
        spin_lock(&MQ_atomic);

        if (0 == sem_timedwait_ms(&ext->queued.sema, 0))
        {
            void *msg = glist_pop(&ext->queued.list);
            glist_push_back(&ext->freed.list, msg);
        }

        spin_unlock(&MQ_atomic);
        return 0;
    }
    else
        return EBADF;
}

int mqueue_queued(int mqd)
{
    if (CID_FD == AsMqd(mqd)->cid && FD_TAG_MQD == (FD_TAG_MQD & AsMqd(mqd)->tag))
    {
        int retval;
        sem_getvalue(AsMqd(mqd)->read_rdy, &retval);
        return retval;
    }
    else
        return __set_errno_neg(EBADF);
}

ssize_t mqueue_recv(int mqd, void *msg, unsigned int *prio)
{
    uint32_t timeo;
    if (! (FD_FLAG_NONBLOCK & AsFD(mqd)->flags))
    {
        timeo = AsFD(mqd)->read_timeo;
        if (0 == timeo)
            timeo = INFINITE;
    }
    else
        timeo = 0;

    return mqueue_timedrecv(mqd, msg, timeo, prio);
}

ssize_t mqueue_timedrecv(int mqd, void *restrict msg, uint32_t timeout, unsigned int *restrict prio)
{
    if (CID_FD == AsMqd(mqd)->cid && FD_TAG_MQD == (FD_TAG_MQD & AsMqd(mqd)->tag))
    {
        struct MQ_ext *ext = AsMqd(mqd)->ext;
        int retval = sem_timedwait_ms(&ext->queued.sema, timeout);

        if (0 == retval)
        {
            struct MQ_msg *que;
            que = SVC_mqueue_get(&ext->queued, AsFD(mqd)->ext);

            if (prio) *prio = que->prio;
            memcpy(msg, que->payload, ext->msg_size);

            SVC_mqueue_release(&ext->freed, que);
            return (ssize_t)ext->msg_size;
        }
        else
            return __set_errno_neg(retval);
    }
    else
        return __set_errno_neg(EBADF);
    }

ssize_t mqueue_send(int mqd, void const *msg, unsigned int prio)
{
    uint32_t timeo;
    if (! (FD_FLAG_NONBLOCK & AsFD(mqd)->flags))
    {
        timeo = AsFD(mqd)->write_timeo;
        if (0 == timeo)
            timeo = INFINITE;
    }
    else
        timeo = 0;

    return mqueue_timedsend(mqd, msg, timeo, prio);
}

ssize_t mqueue_timedsend(int mqd, void const *msg, uint32_t timeout, unsigned int prio)
{
    if (CID_FD == AsMqd(mqd)->cid && FD_TAG_MQD == (FD_TAG_MQD & AsMqd(mqd)->tag))
    {
        struct MQ_ext *ext = AsMqd(mqd)->ext;
        int retval = sem_timedwait_ms(&ext->freed.sema, timeout);

        if (0 == retval)
        {
            struct MQ_msg *que = SVC_mqueue_get(&ext->freed, AsFD(mqd)->ext);

            que->prio = prio;
            memcpy(que->payload, msg, ext->msg_size);

            SVC_mqueue_post(&ext->queued, que, AsFD(mqd)->ext);
            return (ssize_t)ext->msg_size;
        }
        else
            return __set_errno_neg(retval);
    }
    else
        return __set_errno_neg(EBADF);
}

/***************************************************************************/
/** @implements :posix compatiable
****************************************************************************/
mqd_t mq_open(char const *name, int flags, ...)
{
    if (O_CREAT & flags)
    {
        va_list vl;
        va_start(vl, flags);

        /* mode_t mode = */va_arg(vl, mode_t);
        struct mq_attr *attr = va_arg(vl, struct mq_attr *);
        va_end(vl);

        return mqueue_create(name, (uint16_t)attr->mq_msgsize, (uint16_t)attr->mq_maxmsg);
    }
    else
        return SVC_mq_find(name, false);
}

int mq_close(mqd_t mqd)
{
    ARG_UNUSED(mqd);
    // ultracore shared memorty of mqd
    //  destroying the mqd only need to call mq_unlink
    return 0;
}

int mq_unlink(char const *name)
{
    int retval = SVC_mq_find(name, true);

    if (-1 != retval)
        retval = close(retval);

    return retval;
}

int mq_getattr(mqd_t mqd, struct mq_attr *attr)
{
    if (CID_FD == AsMqd(mqd)->cid && FD_TAG_MQD == (FD_TAG_MQD & AsMqd(mqd)->tag))
    {
        struct MQ_ext *ext = AsMqd(mqd)->ext;

        attr->mq_flags = (FD_FLAG_NONBLOCK & AsMqd(mqd)->flags ? O_NONBLOCK : 0);
        attr->mq_msgsize = ext->msg_size;
        attr->mq_maxmsg = ext->msg_max;
        attr->mq_curmsgs = (unsigned)mqueue_queued(mqd);

        return 0;
    }
    else
        return EBADF;
}

int mq_setattr(mqd_t mqd, struct mq_attr const *restrict attr, struct mq_attr *restrict oattr)
{
    ARG_UNUSED(mqd, attr, oattr);
    return __set_errno_neg(ENOSYS);
}

int mq_notify(mqd_t mqd, struct sigevent const *notification)
{
    ARG_UNUSED(mqd, notification);
    return __set_errno_neg(ENOSYS);
}

ssize_t mq_receive(mqd_t mqd, void *buf, size_t bufsize, unsigned int *prio)
{
    struct MQ_ext *ext = AsMqd(mqd)->ext;
    if (bufsize < ext->msg_size)
        return __set_errno_neg(EMSGSIZE);
    else
        return mqueue_recv(mqd, buf, prio);
}

ssize_t mq_timedreceive(mqd_t mqd, void *restrict buf, size_t bufsize, unsigned int *restrict prio,
    struct timespec const *restrict ts)
{
    struct MQ_ext *ext = AsMqd(mqd)->ext;
    if (bufsize < ext->msg_size)
        return __set_errno_neg(EMSGSIZE);

    time_t t = time(NULL);
    if (ts->tv_sec < t)
        return __set_errno_neg(ETIMEDOUT);

    return mqueue_timedrecv(mqd, buf,
        (uint32_t)((ts->tv_sec - 1) * 1000 + (ts->tv_nsec / 1000)),
        prio
    );
}

int mq_send(mqd_t mqd, void const *buf, size_t count, unsigned int prio)
{
    struct MQ_ext *ext = AsMqd(mqd)->ext;
    if (count < ext->msg_size)
        return __set_errno_neg(EMSGSIZE);
    else
        return mqueue_send(mqd, buf, prio);
}

int mq_timedsend(mqd_t mqd, void const *buf, size_t count, unsigned int prio,
    struct timespec const *ts)
{
    struct MQ_ext *ext = AsMqd(mqd)->ext;
    if (count < ext->msg_size)
        return __set_errno_neg(EMSGSIZE);

    time_t t = time(NULL);
    if (ts->tv_sec < t)
        return __set_errno_neg(ETIMEDOUT);

    return mqueue_timedsend(mqd, buf,
        (uint32_t)((ts->tv_sec - 1) * 1000 + (time_t)ts->tv_nsec / 1000),
        prio);
}

/***************************************************************************/
/** @internal
***************************************************************************/
static int SVC_mq_find(char const *name, bool extract)
{
    if (NULL == name)
        return -1;

    int retval = -1;
    spin_lock(&MQ_atomic);

    for (struct KERNEL_mqd **iter = glist_iter_begin(&mq_named_list);
        iter != glist_iter_end(&mq_named_list);
        iter = glist_iter_next(&mq_named_list, iter))
    {
        if (0 == strcmp((*iter)->name, name))
        {
            retval = (int)(*iter);

            if (0 != extract)
                glist_iter_extract(&mq_named_list, iter);

            break;
        }
    }
    spin_unlock(&MQ_atomic);

    if (-1 == retval)
        return __set_errno_neg(ENOENT);
    else
        return retval;
}

static struct MQ_msg *SVC_mqueue_get(struct MQ_list *queue, struct MQ_ext *ext)
{
    spin_lock(&MQ_atomic);

    struct MQ_msg *retval = glist_pop(&queue->list);

    int queued_count;
    sem_getvalue(&queue->sema, &queued_count);

    if (&ext->queued == queue && 0 == queued_count)
        ext->lowest_prio_queued = 0;

    spin_unlock(&MQ_atomic);
    return retval;
}

static void SVC_mqueue_post(struct MQ_list *queue, struct MQ_msg *msg, struct MQ_ext *ext)
{
    struct MQ_msg **iter;
    spin_lock(&MQ_atomic);

    if (ext->lowest_prio_queued <= msg->prio)
    {
        ext->lowest_prio_queued = msg->prio;
        glist_push_back(&queue->list, msg);

        goto mqueue_sem_post;
    }

    iter = (struct MQ_msg **)glist_iter_begin(&queue->list);
    if ((*iter)->prio > msg->prio)
    {
        glist_push_front(&queue->list, msg);
        goto mqueue_sem_post;
    }

    while (iter != glist_iter_end(&queue->list))
    {
        if ((*iter)->prio > msg->prio)
        {
            glist_iter_insert(&queue->list, iter, msg);
            goto mqueue_sem_post;
        }
        else
            iter = glist_iter_next(&queue->list, iter);
    }

    if (false)
    {
mqueue_sem_post:
        sem_post(&queue->sema);
    }
    spin_unlock(&MQ_atomic);
}

static void SVC_mqueue_release(struct MQ_list *queue, struct MQ_msg *msg)
{
    spin_lock(&MQ_atomic);

    glist_push_front(&queue->list, msg);
    sem_post(&queue->sema);

    spin_unlock(&MQ_atomic);
}

/***************************************************************************/
/** @internal: fd io
***************************************************************************/
static ssize_t mqd_read(int mqd, void *buf, size_t bufsize)
{
    if (FD_TAG_MQD != (FD_TAG_MQD & AsMqd(mqd)->tag))
        return EBADF;

    struct MQ_ext *ext = AsMqd(mqd)->ext;

    if (bufsize < ext->msg_size)
        return __set_errno_neg(EMSGSIZE);
    else
        return mqueue_recv(mqd, buf, NULL);
}

static ssize_t mqd_write(int mqd, void const *msg, size_t count)
{
    if (FD_TAG_MQD != (FD_TAG_MQD & AsMqd(mqd)->tag))
        return EBADF;

    struct MQ_ext *ext = AsMqd(mqd)->ext;

    if (count < ext->msg_size)
        return __set_errno_neg(EMSGSIZE);
    else
        return mqueue_send(mqd, msg, 0);
}

static int mqd_close(int mqd)
{
    if (FD_TAG_MQD != (FD_TAG_MQD & AsMqd(mqd)->tag))
        return EBADF;

    AsFD(mqd)->read_rdy = AsFD(mqd)->write_rdy = INVALID_HANDLE;
    KERNEL_mfree(AsMqd(mqd)->ext);

    return 0;
}
