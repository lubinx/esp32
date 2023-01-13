#include <sys/lock.h>

#include "esp_cpu.h"
#include "esp_log.h"
#include "ultracore/atomic.h"

static sys_static_lock_t atomic_lock = {0};

void ATOMIC_enter(void)
{
    __retarget_lock_acquire(&atomic_lock);
}

void ATOMIC_leave(void)
{
    __retarget_lock_release_recursive(&atomic_lock);
}
