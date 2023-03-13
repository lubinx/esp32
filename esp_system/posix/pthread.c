
#include <errno.h>
#include <time.h>
#include <pthread.h>
#include <sched.h>
#include <string.h>

#include <sys/errno.h>
#include <sys/mutex.h>

#include "esp_attr.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"

struct __thread_arg
{
    StaticTask_t task;

    void *(*start_routine)(void *arg);
    void *arg;
    void *exit_code;

    uint32_t stack[];
};

static int get_default_pthread_core(void)
{
    return CONFIG_PTHREAD_TASK_CORE_DEFAULT == -1 ? tskNO_AFFINITY : CONFIG_PTHREAD_TASK_CORE_DEFAULT;
}

static void __freertos_thread_entry(struct __thread_arg *thread)
{
    thread->exit_code = thread->start_routine(thread->arg);
    // vTaskDelete(&thread->task);
}

int pthread_create(pthread_t *thread, pthread_attr_t const *attr, pthread_routine_t routine, void *arg)
{
    uint32_t stack_size = attr ? attr->stack_size : CONFIG_PTHREAD_TASK_STACK_SIZE_DEFAULT;
    BaseType_t prio = CONFIG_PTHREAD_TASK_PRIO_DEFAULT;

    struct __thread_arg *param = malloc(sizeof(struct __thread_arg) + stack_size);
    param->start_routine = routine;
    param->arg = arg;
    param->exit_code = NULL;

    *thread = xTaskCreateStatic((void *)__freertos_thread_entry, NULL,
        stack_size, param, prio,
        (void *)&param->stack, &param->task
    );
}

int pthread_join(pthread_t thread, void **retval)
{
    return 0;
}

int pthread_detach(pthread_t thread)
{
    return 0;
}

int pthread_cancel(pthread_t thread)
{
    return ENOSYS;
}

pthread_t pthread_self(void)
{
    return xTaskGetCurrentTaskHandle();
}

int pthread_equal(pthread_t t1, pthread_t t2)
{
    return ((uintptr_t)t1 - (uintptr_t)t2);
}

int pthread_setcancelstate(int state, int *oldstate)
{
    ARG_UNUSED(state, oldstate);
    return ENOSYS;
}

int pthread_setcanceltype(int type, int *old_type)
{
    ARG_UNUSED(type, old_type);
    return ENOSYS;
}

/***************************************************************************
 *  @implements: pthread attr
 ***************************************************************************/
int pthread_attr_init(pthread_attr_t *attr)
{
    memset(attr, 0, sizeof(*attr));
    attr->stack_size   = CONFIG_PTHREAD_TASK_STACK_SIZE_DEFAULT;
    attr->detachstate = PTHREAD_CREATE_JOINABLE;

    return 0;
}

int pthread_attr_destroy(pthread_attr_t *attr)
{
    return 0;
}

int pthread_attr_getstacksize(pthread_attr_t const *attr, size_t *stacksize)
{
    *stacksize = attr->stack_size;
    return 0;
}

int pthread_attr_setstacksize(pthread_attr_t *attr, size_t stacksize)
{
    if (stacksize < CONFIG_PTHREAD_STACK_MIN)
        return EINVAL;

    attr->stack_size = stacksize;
    return 0;
}

int pthread_attr_getdetachstate(pthread_attr_t const *attr, int *detachstate)
{
    *detachstate = attr->detachstate;
    return 0;
}

int pthread_attr_setdetachstate(pthread_attr_t *attr, int detachstate)
{
    attr->detachstate = detachstate;
    return 0;
}

/***************************************************************************
 *  @implements: pthread key
 ***************************************************************************/
int pthread_key_create(pthread_key_t *key, void (*destructor)(void *))
{
    *key = malloc(sizeof(pthread_key_t));
    if (*key)
    {
        (*key)->val = 0;
        (*key)->destructor = destructor;
    }
    return 0;
}

int pthread_key_delete(pthread_key_t key)
{
    key->destructor();
    free(key);
}

void *pthread_getspecific(pthread_key_t key)
{
    return key->val;
}

int pthread_setspecific(pthread_key_t key, void const *val)
{
    key->val = (void *)val;
    return 0;
}

/***************************************************************************
 *  @implements
 ***************************************************************************/
int pthread_spin_init(pthread_spinlock_t *lock, int pshared)
{
    ARG_UNUSED(pshared);

    lock->owner = 0;
    return 0;
}

int pthread_spin_destroy(pthread_spinlock_t *lock)
{
    ARG_UNUSED(lock);
    return 0;
}

int pthread_spin_lock(pthread_spinlock_t *lock)
{
    pthread_t curr = pthread_self();

    if (lock->owner == curr)
        return EDEADLK;

    while (! __sync_bool_compare_and_swap(&lock->owner, 0, (uint32_t)curr))
        sched_yield();

    return 0;
}

int pthread_spin_trylock(pthread_spinlock_t *lock)
{
    pthread_t curr = pthread_self();

    if (lock->owner == curr)
        return EDEADLK;

    if (__sync_bool_compare_and_swap((uint32_t *)&lock->owner, 0, (uint32_t)curr))
        return 0;
    else
        return EBUSY;
}

int pthread_spin_unlock(pthread_spinlock_t *lock)
{
    if (lock->owner == pthread_self())
    {
        lock->owner = 0;
        return 0;
    }
    else
        return EPERM;
}

/***************************************************************************
 *  @implements: pthread mutex
 ***************************************************************************/
int pthread_mutex_init(pthread_mutex_t *mutex, pthread_mutexattr_t const *attr)
{
    mutex_t *retval = mutex_create(attr == NULL ||
        PTHREAD_MUTEX_RECURSIVE == attr->type ? MUTEX_FLAG_RECURSIVE : MUTEX_FLAG_NORMAL);

    if (retval)
    {
        *mutex = (pthread_mutex_t)retval;
        return 0;
    }
    else
        return ENOMEM;
}

int pthread_mutex_destroy(pthread_mutex_t *mutex)
{
    if (PTHREAD_MUTEX_INITIALIZER != *mutex && PTHREAD_RECURSIVE_MUTEX_INITIALIZER != *mutex)
       return mutex_destroy((mutex_t *)*mutex);
    else
        return 0;
}

static void IRAM_ATTR pthread_mutex_do_initializer(pthread_mutex_t *mutex)
{
    static spinlock_t atomic = SPINLOCK_INITIALIZER;
    spin_lock(&atomic);

    if (PTHREAD_MUTEX_INITIALIZER == *mutex)
        *mutex = (pthread_mutex_t)mutex_create(MUTEX_FLAG_NORMAL);
    else if(PTHREAD_RECURSIVE_MUTEX_INITIALIZER == *mutex)
        *mutex = (pthread_mutex_t)mutex_create(MUTEX_FLAG_RECURSIVE);

    spin_unlock(&atomic);
}

int IRAM_ATTR pthread_mutex_lock(pthread_mutex_t *mutex)
{
    if (PTHREAD_MUTEX_INITIALIZER == *mutex || PTHREAD_RECURSIVE_MUTEX_INITIALIZER == *mutex)
        pthread_mutex_do_initializer(mutex);

    return mutex_lock((mutex_t *)*mutex);
}

int IRAM_ATTR pthread_mutex_trylock(pthread_mutex_t *mutex)
{
    if (PTHREAD_MUTEX_INITIALIZER == *mutex || PTHREAD_RECURSIVE_MUTEX_INITIALIZER == *mutex)
        pthread_mutex_do_initializer(mutex);

    return mutex_trylock((mutex_t *)*mutex, 0);
}

int IRAM_ATTR pthread_mutex_unlock(pthread_mutex_t *mutex)
{
    if (PTHREAD_MUTEX_INITIALIZER != *mutex && PTHREAD_RECURSIVE_MUTEX_INITIALIZER != *mutex)
        return mutex_unlock((mutex_t *)*mutex);
    else
        return EACCES;
}

int pthread_mutex_getprioceiling(const pthread_mutex_t *restrict mutex, int *restrict prioceiling)
{
    ARG_UNUSED(mutex, prioceiling);
    return 0;
}

int pthread_mutex_setprioceiling(pthread_mutex_t *restrict mutex, int prioceiling, int *old_prioceiling)
{
    ARG_UNUSED(mutex, prioceiling, old_prioceiling);
    return 0;
}

/***************************************************************************
 *  @implements: pthread mutex attr
 ***************************************************************************/
int pthread_mutexattr_init(pthread_mutexattr_t *attr)
{
    memset(attr, 0, sizeof(pthread_mutexattr_t));
    return 0;
}

int pthread_mutexattr_destroy(pthread_mutexattr_t *attr)
{
    ARG_UNUSED(attr);
    return 0;
}

int pthread_mutexattr_getprioceiling(pthread_mutexattr_t const *restrict attr, int *restrict prioceiling)
{
    ARG_UNUSED(attr, prioceiling);
    return 0;
}

int pthread_mutexattr_setprioceiling(pthread_mutexattr_t *attr, int prioceiling)
{
    ARG_UNUSED(attr, prioceiling);
    return 0;
}

int pthread_mutexattr_getprotocol(pthread_mutexattr_t const *restrict attr, int *restrict protocol)
{
    ARG_UNUSED(attr);
    *protocol = PTHREAD_PRIO_NONE;
    return 0;
}

int pthread_mutexattr_setprotocol(pthread_mutexattr_t *attr, int protocol)
{
    ARG_UNUSED(attr, protocol);
    return 0;
}

int pthread_mutexattr_getpshared(pthread_mutexattr_t const *restrict attr, int *restrict pshared)
{
    ARG_UNUSED(attr);
    *pshared = PTHREAD_PROCESS_PRIVATE;
    return 0;
}

int pthread_mutexattr_setpshared(pthread_mutexattr_t *attr, int pshared)
{
    ARG_UNUSED(attr, pshared);
    return 0;
}

int pthread_mutexattr_gettype(pthread_mutexattr_t const *restrict attr, int *restrict type)
{
    *type = attr->type;
    return 0;
}

int pthread_mutexattr_settype(pthread_mutexattr_t *attr, int type)
{
    attr->type = type;
    return 0;
}
