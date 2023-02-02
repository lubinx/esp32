#include <sys/lock.h>

#include "esp_cpu.h"
#include "esp_log.h"
#include "ultracore/atomic.h"

static char const *TAG = "atomic";
static sys_static_lock_t atomic_lock = {0};

__attribute__((constructor))
static void ATOMIC_init(void)
{
    ESP_LOGD(TAG, "constructor: ATOMIC_init()");
    libc_lock_sinit(&atomic_lock);
}

void ATOMIC_enter(void)
{
    __retarget_lock_acquire(&atomic_lock);
}

void ATOMIC_leave(void)
{
    __retarget_lock_release_recursive(&atomic_lock);
}
