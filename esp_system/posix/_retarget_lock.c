/*
 * SPDX-FileCopyrightText: 2015-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <sys/lock.h>
#include <stdlib.h>
#include <sys/reent.h>

#include "esp_log.h"
#include "esp_rom_caps.h"

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

// requirement checking
static_assert(sizeof(struct __lock) != sizeof(StaticSemaphore_t), "sizeof(struct __lock) != sizeof(StaticSemaphore_t)");
static_assert(configSUPPORT_STATIC_ALLOCATION, "FreeRTOS should be configured with static allocation support");

static struct __lock    __sinit_recursive_mutex = {0};
static struct __lock    __malloc_recursive_mutex = {0};
static struct __lock    __env_recursive_mutex = {0};
static struct __lock    __sfp_recursive_mutex = {0};
static struct __lock    __atexit_recursive_mutex = {0};
static struct __lock    __at_quick_exit_mutex = {0};
static struct __lock    __tz_mutex = {0};
static struct __lock    __dd_hash_mutex = {0};
static struct __lock    __arc4random_mutex = {0};

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

    static struct __lock    s_common_mutex = {0};
    static struct __lock    s_common_recursive_mutex = {0};
#endif

/* somehow esp-idf libc.a already defined these, this is how to override it */
extern struct __lock    __lock___sinit_recursive_mutex  __attribute__((alias("__sinit_recursive_mutex")));
extern struct __lock    __lock___malloc_recursive_mutex __attribute__((alias("__malloc_recursive_mutex")));
extern struct __lock    __lock___env_recursive_mutex    __attribute__((alias("__env_recursive_mutex")));
extern struct __lock    __lock___sfp_recursive_mutex    __attribute__((alias("__sfp_recursive_mutex")));
extern struct __lock    __lock___atexit_recursive_mutex __attribute__((alias("__atexit_recursive_mutex")));
extern struct __lock    __lock___at_quick_exit_mutex    __attribute__((alias("__at_quick_exit_mutex")));
extern struct __lock    __lock___tz_mutex               __attribute__((alias("__tz_mutex")));
extern struct __lock    __lock___dd_hash_mutex          __attribute__((alias("__dd_hash_mutex")));
extern struct __lock    __lock___arc4random_mutex       __attribute__((alias("__arc4random_mutex")));

void __LOCK_retarget(void)
{
    xSemaphoreCreateRecursiveMutexStatic((void *)&__lock___sinit_recursive_mutex.__dummy);
    xSemaphoreCreateRecursiveMutexStatic((void *)&__lock___sfp_recursive_mutex.__dummy);
    xSemaphoreCreateRecursiveMutexStatic((void *)&__lock___env_recursive_mutex.__dummy);
    xSemaphoreCreateRecursiveMutexStatic((void *)&__lock___malloc_recursive_mutex.__dummy);
    xSemaphoreCreateRecursiveMutexStatic((void *)&__lock___atexit_recursive_mutex.__dummy);

    xSemaphoreCreateMutexStatic((void *)&__lock___at_quick_exit_mutex.__dummy);
    xSemaphoreCreateMutexStatic((void *)&__lock___tz_mutex.__dummy);
    xSemaphoreCreateMutexStatic((void *)&__lock___dd_hash_mutex.__dummy);
    xSemaphoreCreateMutexStatic((void *)&__lock___arc4random_mutex.__dummy);

    #if ESP_ROM_HAS_RETARGETABLE_LOCKING
        xSemaphoreCreateMutexStatic((void *)&s_common_mutex.__dummy);
        xSemaphoreCreateMutexStatic((void *)&s_common_recursive_mutex.__dummy);
    #endif
}

/****************************************************************************
 * @implements: static locks
*****************************************************************************/
void libc_lock_init(_LOCK_T *lock)
{
    *lock = (void *)xSemaphoreCreateMutex();
}

void libc_lock_init_recursive(_LOCK_T *lock)
{
    *lock = (void *)xSemaphoreCreateRecursiveMutex();
}

void libc_lock_sinit(_LOCK_T lock)
{
    xSemaphoreCreateMutexStatic((void *)&lock->__dummy);
}

void libc__lock_sinit_recursive(_LOCK_T lock)
{
    xSemaphoreCreateRecursiveMutexStatic((void *)&lock->__dummy);
}

void libc_lock_close(_LOCK_T lock)
{
    vSemaphoreDelete(lock->__dummy);
}

void libc_lock_acquire(_LOCK_T lock)
{
    if (! xPortCanYield())
    {
        BaseType_t higher_task_woken = false;
        if (! xSemaphoreTakeFromISR((void *)lock, &higher_task_woken))
        {
            assert(higher_task_woken);
            portYIELD_FROM_ISR();
        }
    }
    else
        xSemaphoreTake((void *)lock, portMAX_DELAY);
}

void libc_lock_acquire_recursive(_LOCK_T lock)
{
    xSemaphoreTakeRecursive((void *)lock, portMAX_DELAY);
}


int libc_lock_try_acquire(_LOCK_T lock)
{
    if (pdTRUE == xSemaphoreTake((void *)lock, 0))
        return 0;
    else
        return EBUSY;
}

int libc_lock_try_acquire_recursive(_LOCK_T lock)
{
    if (pdTRUE == xSemaphoreTakeRecursive((void *)lock, 0))
        return 0;
    else
        return EBUSY;
}

void libc_lock_release(_LOCK_T lock)
{
    if (! xPortCanYield())
    {
        BaseType_t higher_task_woken = false;
        xSemaphoreGiveFromISR((void *)lock, &higher_task_woken);

        if (higher_task_woken)
            portYIELD_FROM_ISR();
    }
    else
        xSemaphoreGive((void *)lock);
}

void libc_lock_release_recursive(_LOCK_T lock)
{
    xSemaphoreGiveRecursive((void *)lock);
}

void __retarget_lock_init(_LOCK_T *lock)
    __attribute__((alias("libc_lock_init")));

void __retarget_lock_init_recursive(_LOCK_T *lock)
    __attribute__((alias("libc_lock_init_recursive")));

void libc_lock_close_recursive(_LOCK_T lock)
    __attribute__((alias("libc_lock_close")));

void __retarget_lock_close(_LOCK_T lock)
    __attribute__((alias("libc_lock_close")));

void __retarget_lock_close_recursive(_LOCK_T lock)
    __attribute__((alias("libc_lock_close")));

/****************************************************************************
 * @implements: newlib retargeting
*****************************************************************************/
void __retarget_lock_acquire(_LOCK_T lock)
{
#if ESP_ROM_HAS_RETARGETABLE_LOCKING
    if (ROM_MUTEX_MAGIC == *(int*)lock)
        lock = &s_common_mutex;
#endif
    _lock_acquire(&lock);
}

void __retarget_lock_acquire_recursive(_LOCK_T lock)
{
#if ESP_ROM_HAS_RETARGETABLE_LOCKING
    if (ROM_MUTEX_MAGIC == *(int *)lock)
        lock = &s_common_recursive_mutex;
#endif
    _lock_acquire_recursive(&lock);
}

int __retarget_lock_try_acquire(_LOCK_T lock)
{
#if ESP_ROM_HAS_RETARGETABLE_LOCKING
    if (ROM_MUTEX_MAGIC == *(int *)lock)
        lock = &s_common_mutex;
#endif
    return _lock_try_acquire(&lock);
}

int __retarget_lock_try_acquire_recursive(_LOCK_T lock)
{
#if ESP_ROM_HAS_RETARGETABLE_LOCKING
    if (ROM_MUTEX_MAGIC == *(int *)lock)
        lock = &s_common_recursive_mutex;
#endif
    return _lock_try_acquire_recursive(&lock);
}

void __retarget_lock_release(_LOCK_T lock)
{
#if ESP_ROM_HAS_RETARGETABLE_LOCKING
    if (ROM_MUTEX_MAGIC == *(int *)lock)
        lock = &s_common_mutex;
#endif
    _lock_release(&lock);
}

void __retarget_lock_release_recursive(_LOCK_T lock)
{
#if ESP_ROM_HAS_RETARGETABLE_LOCKING
    if (ROM_MUTEX_MAGIC == *(int*)lock)
        lock = &s_common_recursive_mutex;
#endif
    _lock_release_recursive(&lock);
}

/****************************************************************************
 * @implements: esp-idf
*****************************************************************************/
int __esp_lock_impl(_LOCK_T *lock, int (*libc_lock_func)(_LOCK_T lock), char const *__function__)
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
        return libc_lock_func((void *)hdl);
    }
}
