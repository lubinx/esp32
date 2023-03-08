#include <string.h>
#include <stdlib.h>

#include <sys/lock.h>
#include <sys/errno.h>
#include <sys/mutex.h>

#include "esp_log.h"

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

static spinlock_t mutex_atomic = SPINLOCK_INITIALIZER;
#define MUTEX_FLAG_STATICALLY           (0x80)

struct __lock
{
    mutex_t mutex;
};

// requirement checking
static_assert(sizeof_member(struct __mutex_t, __pad) != sizeof(StaticSemaphore_t), "sizeof(struct __lock) != sizeof(StaticSemaphore_t)");
static_assert(configSUPPORT_STATIC_ALLOCATION, "FreeRTOS should be configured with static allocation support");

struct __lock    __lock___sinit_recursive_mutex     = {0};
struct __lock    __lock___malloc_recursive_mutex    = {0};
struct __lock    __lock___env_recursive_mutex       = {0};
struct __lock    __lock___sfp_recursive_mutex       = {0};
struct __lock    __lock___atexit_recursive_mutex    = {0};
struct __lock    __lock___at_quick_exit_mutex       = {0};
struct __lock    __lock___tz_mutex                  = {0};
struct __lock    __lock___dd_hash_mutex             = {0};
struct __lock    __lock___arc4random_mutex          = {0};

#if ESP_ROM_HAS_RETARGETABLE_LOCKING
    /* C3 and S3 ROMs are built without Newlib static lock symbols exported, and
    * with an extra level of _LOCK_T indirection in mind.
    * The following is a workaround for this:
    * - on startup, we call esp_rom_newlib_init_common_mutexes to set
    *   the two mutex pointers to magic values.
    * - if in __retarget_lock_acquire*, we check if the argument dereferences
    *   to the magic value. If yes, we lock the correct mutex defined in the app,
    *   instead.
    * Casts from &StaticSemaphore_t to _LOCK_T are okay because _LOCK_T
    * (which is SemaphoreHandle_t) is a pointer to the corresponding
    * StaticSemaphore_t structure.
    */
    #define ROM_MUTEX_MAGIC             0xbb10c433

    static struct __lock    idf_common_mutex = {0};
    static struct __lock    idf_common_recursive_mutex = {0};
#endif

void __LOCK_retarget_init(void)
{
    __lock___sinit_recursive_mutex.mutex.__init = MUTEX_FLAG_RECURSIVE;
    __lock___sfp_recursive_mutex.mutex.__init = MUTEX_FLAG_RECURSIVE;
    __lock___env_recursive_mutex.mutex.__init = MUTEX_FLAG_RECURSIVE;
    __lock___malloc_recursive_mutex.mutex.__init = MUTEX_FLAG_RECURSIVE;
    __lock___atexit_recursive_mutex.mutex.__init = MUTEX_FLAG_RECURSIVE;
    xSemaphoreCreateRecursiveMutexStatic((void *)&__lock___sinit_recursive_mutex.mutex.__pad);
    xSemaphoreCreateRecursiveMutexStatic((void *)&__lock___sfp_recursive_mutex.mutex.__pad);
    xSemaphoreCreateRecursiveMutexStatic((void *)&__lock___env_recursive_mutex.mutex.__pad);
    xSemaphoreCreateRecursiveMutexStatic((void *)&__lock___malloc_recursive_mutex.mutex.__pad);
    xSemaphoreCreateRecursiveMutexStatic((void *)&__lock___atexit_recursive_mutex.mutex.__pad);

    __lock___at_quick_exit_mutex.mutex.__init = MUTEX_FLAG_NORMAL;
    __lock___tz_mutex.mutex.__init = MUTEX_FLAG_NORMAL;
    __lock___dd_hash_mutex.mutex.__init = MUTEX_FLAG_NORMAL;
    __lock___arc4random_mutex.mutex.__init = MUTEX_FLAG_NORMAL;
    xSemaphoreCreateMutexStatic((void *)&__lock___at_quick_exit_mutex.mutex.__pad);
    xSemaphoreCreateMutexStatic((void *)&__lock___tz_mutex.mutex.__pad);
    xSemaphoreCreateMutexStatic((void *)&__lock___dd_hash_mutex.mutex.__pad);
    xSemaphoreCreateMutexStatic((void *)&__lock___arc4random_mutex.mutex.__pad);

    #if ESP_ROM_HAS_RETARGETABLE_LOCKING
        idf_common_mutex.__init = MUTEX_FLAG_RECURSIVE;
        xSemaphoreCreateRecursiveMutexStatic((void *)&idf_common_recursive_mutex.__pad);

        idf_common_mutex.__init = MUTEX_FLAG_NORMAL;
        xSemaphoreCreateMutexStatic((void *)&idf_common_mutex.__pad);
    #endif
}

/****************************************************************************
 * @implements: mutex
*****************************************************************************/
mutex_t *mutex_create(int flags)
{
    mutex_t *retval = malloc(sizeof(mutex_t));

    if (retval)
    {
        if (MUTEX_FLAG_RECURSIVE == flags)
            xSemaphoreCreateRecursiveMutexStatic((void *)&retval->__pad);
        else
            xSemaphoreCreateMutexStatic((void *)&retval->__pad);

        retval->__init = flags;
        return retval;
    }
    else
        return __set_errno_nullptr(ENOMEM);
}

int mutex_destroy(mutex_t *mutex)
{
    if (MUTEX_FLAG_STATICALLY & mutex->__init)
    {
        vSemaphoreDelete((void *)mutex);
        free(mutex);
    }
    return 0;
}

int mutex_init(mutex_t *mutex, int flags)
{
    if (MUTEX_FLAG_RECURSIVE == flags)
        xSemaphoreCreateRecursiveMutexStatic((void *)&mutex->__pad);
    else
        xSemaphoreCreateMutexStatic((void *)&mutex->__pad);

    mutex->__init = MUTEX_FLAG_STATICALLY | flags;
    return 0;
}

int mutex_lock(mutex_t *mutex)
{
    return mutex_trylock(mutex, portMAX_DELAY);
}

int mutex_trylock(mutex_t *mutex, uint32_t timeout)
{
    if (0 != __get_IPSR())  // mutex not allowed in ISR
        return EPERM;

    int __init;
    spin_lock(&mutex_atomic);

    if (mutex->__init < 0)
    {
        mutex_init(mutex, ~mutex->__init);
        __init = mutex->__init;
    }
    spin_unlock(&mutex_atomic);

    if (MUTEX_FLAG_RECURSIVE & __init)
        xSemaphoreTake((void *)&mutex->__pad, timeout);
    else
        xSemaphoreTakeRecursive((void *)&mutex->__pad, timeout);

    return 0;
}

int mutex_unlock(mutex_t *mutex)
{
    if (mutex->__init < 0)  // not possiable fall here
        return EPERM;

    if (MUTEX_FLAG_RECURSIVE & mutex->__init)
        xSemaphoreGiveRecursive((void *)&mutex->__pad);
    else
        xSemaphoreGive((void *)&mutex->__pad);

    return 0;
}

/****************************************************************************
 * @implements: newlib retargeting
*****************************************************************************/
void __retarget_lock_init(_LOCK_T *lock)
{
    *lock = (_LOCK_T)mutex_create(MUTEX_FLAG_NORMAL);
}

void __retarget_lock_init_recursive(_LOCK_T *lock)
{
    *lock = (_LOCK_T)mutex_create(MUTEX_FLAG_RECURSIVE);
}

#pragma GCC diagnostic ignored "-Wattribute-alias"

void __retarget_lock_close(_LOCK_T lock)
    __attribute__((alias("mutex_destroy")));

void __retarget_lock_close_recursive(_LOCK_T lock)
    __attribute__((alias("mutex_destroy")));

void __retarget_lock_acquire(_LOCK_T lock)
{
#if ESP_ROM_HAS_RETARGETABLE_LOCKING
    if (ROM_MUTEX_MAGIC == *(int*)lock)
        lock = &idf_common_mutex;
#endif
    _lock_acquire(&lock);
}

void __retarget_lock_acquire_recursive(_LOCK_T lock)
{
#if ESP_ROM_HAS_RETARGETABLE_LOCKING
    if (ROM_MUTEX_MAGIC == *(int *)lock)
        lock = &idf_common_recursive_mutex;
#endif
    _lock_acquire_recursive(&lock);
}

int __retarget_lock_try_acquire(_LOCK_T lock)
{
#if ESP_ROM_HAS_RETARGETABLE_LOCKING
    if (ROM_MUTEX_MAGIC == *(int *)lock)
        lock = &idf_common_mutex;
#endif
    return _lock_try_acquire(&lock);
}

int __retarget_lock_try_acquire_recursive(_LOCK_T lock)
{
#if ESP_ROM_HAS_RETARGETABLE_LOCKING
    if (ROM_MUTEX_MAGIC == *(int *)lock)
        lock = &idf_common_recursive_mutex;
#endif
    return _lock_try_acquire_recursive(&lock);
}

void __retarget_lock_release(_LOCK_T lock)
{
#if ESP_ROM_HAS_RETARGETABLE_LOCKING
    if (ROM_MUTEX_MAGIC == *(int *)lock)
        lock = &idf_common_mutex;
#endif
    _lock_release(&lock);
}

void __retarget_lock_release_recursive(_LOCK_T lock)
{
#if ESP_ROM_HAS_RETARGETABLE_LOCKING
    if (ROM_MUTEX_MAGIC == *(int*)lock)
        lock = &idf_common_recursive_mutex;
#endif
    _lock_release_recursive(&lock);
}

/****************************************************************************
 * @implements: esp-idf
*****************************************************************************/
static int __esp_lock_impl(_LOCK_T *lock, int (*mutex_func)(mutex_t *), char const *__function__)
{
    if (taskSCHEDULER_NOT_STARTED == xTaskGetSchedulerState())
    {
        /**
         *  somehow they make this happen in esp-idf source code
         *      this is ...ok when freertos task scheduler is not running, none thread was dispatching
         *  REVIEW: add a atomic lock?
        */
        ESP_LOGW("libc", "%s() before freertos scheduler startup...%p", __function__, *lock);
    }
    else
    {
        SemaphoreHandle_t hdl = (SemaphoreHandle_t)(*lock);
        assert(NULL != hdl);
        /*
        if (! hdl)
        {
            ESP_LOGE("libc", "lock acquire CREATE... %p", lock);

            hdl = xSemaphoreCreateMutex();
            assert(NULL != hdl);

            *lock = (void *)hdl;
        }
        */
        return mutex_func((void *)hdl);
    }
}

static int __mutex_trylock(mutex_t *mutex)
{
    return mutex_trylock(mutex, 0);
}

void _lock_acquire(_LOCK_T *lock)
{
    __esp_lock_impl(lock, &mutex_lock, __func__);
}

void _lock_acquire_recursive(_LOCK_T *lock)
{
    __esp_lock_impl(lock, &mutex_lock, __func__);
}

int _lock_try_acquire(_LOCK_T *lock)
{
    return __esp_lock_impl(lock, &__mutex_trylock, __func__);
}

int _lock_try_acquire_recursive(_LOCK_T *lock)
{
    return __esp_lock_impl(lock, &__mutex_trylock, __func__);
}

void _lock_release(_LOCK_T *lock)
{
    __esp_lock_impl(lock, &mutex_unlock, __func__);
}

void _lock_release_recursive(_LOCK_T *lock)
{
    __esp_lock_impl(lock, &mutex_unlock, __func__);
}
