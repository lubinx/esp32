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
    mutex_init(&__lock___sinit_recursive_mutex.mutex, MUTEX_FLAG_RECURSIVE);
    mutex_init(&__lock___sfp_recursive_mutex.mutex, MUTEX_FLAG_RECURSIVE);
    mutex_init(&__lock___env_recursive_mutex.mutex, MUTEX_FLAG_RECURSIVE);
    mutex_init(&__lock___malloc_recursive_mutex.mutex, MUTEX_FLAG_RECURSIVE);
    mutex_init(&__lock___atexit_recursive_mutex.mutex, MUTEX_FLAG_RECURSIVE);

    mutex_init(&__lock___at_quick_exit_mutex.mutex, MUTEX_FLAG_NORMAL);
    mutex_init(&__lock___tz_mutex.mutex, MUTEX_FLAG_NORMAL);
    mutex_init(&__lock___dd_hash_mutex.mutex, MUTEX_FLAG_NORMAL);
    mutex_init(&__lock___arc4random_mutex.mutex, MUTEX_FLAG_NORMAL);

    #if ESP_ROM_HAS_RETARGETABLE_LOCKING
        mutex_init(&idf_common_recursive_mutex, MUTEX_FLAG_RECURSIVE);
        mutex_init(&idf_common_mutex, MUTEX_FLAG_NORMAL);
    #endif
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

void __retarget_lock_close(_LOCK_T lock)
{
    mutex_destroy(&lock->mutex);
}

void __retarget_lock_close_recursive(_LOCK_T lock)
    __attribute__((alias("__retarget_lock_close")));

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
