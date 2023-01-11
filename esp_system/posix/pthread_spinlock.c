#include <pthread.h>

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#if defined(__XTENSA__)
    #include "xtensa/xtruntime.h"
    #include "xt_utils.h"
#elif defined(__riscv)
#else
    #pragma GCC error "pthread: unsupported arch"
#endif

struct pthread_spinlock_t const PTHREAD_SPINLOCK_INITIALIZER =
{
    .owner = PTHREAD_SPINLOCK_NO_OWNER,
    .lock_count = 0
};

int pthread_spin_init(pthread_spinlock_t *lock, int pshared)
{
    ARG_UNUSED(pshared);
    *lock = PTHREAD_SPINLOCK_INITIALIZER;

    return 0;
}

int pthread_spin_destroy(pthread_spinlock_t *lock)
{
    ARG_UNUSED(lock);
    return 0;
}

int pthread_spin_lock(pthread_spinlock_t *lock)
{
#if defined(__XTENSA__)
    uint32_t irq_status = XTOS_SET_INTLEVEL(XCHAL_EXCM_LEVEL);
    uint32_t core_id = xt_utils_get_raw_core_id();
#else
#endif

    lock->lock_count ++;
    return 0;
}


int pthread_spin_trylock(pthread_spinlock_t *lock)
{
}

int pthread_spin_unlock(pthread_spinlock_t *lock)
{
}
