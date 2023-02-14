#include <pthread.h>
#include <stdbool.h>
#include <sys/errno.h>

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
        pthread_yield();

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
