/*
 * SPDX-FileCopyrightText: 2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stddef.h>
#include <assert.h>
#include <sys/types.h>

#include <ultracore/kernel.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/semphr.h>

#include "esp_attr.h"
#include "esp_err.h"
#include "esp_system.h"
#include "esp_heap_caps.h"

#include "clk_tree.h"

#include "hal/systimer_ll.h"
#include "hal/systimer_hal.h"

#include <ultracore/types.h>
#include "sys/mutex.h"
#include "semaphore.h"

#include "sdkconfig.h"
#include "esp_rom_sys.h"

/****************************************************************************
 *  @implements: freertos main task
*****************************************************************************/
static char const *freertos_argv = "freertos_start";

static void __esp_freertos_start(void *arg)
{
    ARG_UNUSED(arg);
    __esp_rtos_initialize();

    extern __attribute__((noreturn)) int main(int argc, char **argv);
    // TODO: process main() exit code
    main(1, (char **)&freertos_argv);

    // main should not return
    vTaskDelete(NULL);
}

void esp_rtos_bootstrap(void)
{
    static uintptr_t __main_stack[CONFIG_ESP_MAIN_TASK_STACK_SIZE / sizeof(uintptr_t)];
    static StaticTask_t __main_task;

    // Initialize the cross-core interrupt on CPU0
    esp_crosscore_int_init();

   if (0 == __get_CORE_ID())
   {
        // TODO: main task pined to core?
        // CONFIG_ESP_MAIN_TASK_AFFINITY

        xTaskCreateStatic(__esp_freertos_start, freertos_argv,
            CONFIG_ESP_MAIN_TASK_STACK_SIZE, NULL, configMAX_PRIORITIES,
            (void *)__main_stack, &__main_task
        );

        vTaskStartScheduler();
        abort();
   }
   else
   {
        xPortStartScheduler();
        abort(); // Only get to here if FreeRTOS somehow very broken
   }
}

__attribute__((weak))
void __esp_rtos_initialize(void)
{
}

/****************************************************************************
 *  @implements: freertos tick & idle
*****************************************************************************/
void IRAM_ATTR vApplicationTickHook(void)
{
    // nothing to do
}

void IRAM_ATTR vApplicationIdleHook(void)
{
    __WFI();
}

/****************************************************************************
 *  @implements: unistd.c sleep() / msleep() / usleep()
*****************************************************************************/
int usleep(useconds_t us)
{
    if (! us)
        return 0;

    uint32_t us_per_tick = portTICK_PERIOD_MS * 1000;
    if (us > us_per_tick)
    {
        vTaskDelay((us + us_per_tick - 1) / us_per_tick);
        return 0;
    }

    esp_rom_delay_us(us);
    return 0;
}

unsigned int sleep(unsigned int seconds)
{
    vTaskDelay(seconds * 1000 / portTICK_PERIOD_MS);
    return 0;
}

int msleep(uint32_t msec)
{
    if (msec)
        vTaskDelay(msec / portTICK_PERIOD_MS);
    else
        taskYIELD();

    return 0;
}

// TODO: move to pthread.c
int pthread_yield(void)
{
    taskYIELD();
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

/****************************************************************************
 * @implements: generic synchronize objects
*****************************************************************************/
int waitfor(handle_t hdl, uint32_t timeout)
{
    UBaseType_t retval;
    if (HDL_FLAG_FREERTOS_RECURSIVE & AsKernelHdl(hdl)->flags)
        retval = xSemaphoreTakeRecursive((void *)&AsKernelHdl(hdl)->padding, timeout / portTICK_PERIOD_MS);
    else
        retval = xSemaphoreTake((void *)&AsKernelHdl(hdl)->padding, timeout / portTICK_PERIOD_MS);

    if (pdTRUE == retval)
        return 0;
    else
        return __set_errno_neg(ETIMEDOUT);
}

static int __freertos_init_hdl(struct KERNEL_hdl *hdl, uint8_t cid, uint8_t flags)
{
    switch (cid)
    {
    case CID_SEMAPHORE:
        xSemaphoreCreateCountingStatic(hdl->init_sem.max_count, hdl->init_sem.initial_count, (void *)hdl->padding);
        break;

    case CID_MUTEX:
        if (HDL_FLAG_FREERTOS_RECURSIVE & flags)
            xSemaphoreCreateRecursiveMutexStatic((void *)hdl->padding);
        else
            xSemaphoreCreateMutexStatic((void *)&hdl->padding);
        break;

    default:
        while (1);
        return ENOSYS;
    }

    hdl->cid = cid;
    hdl->flags = flags & ~(HDL_FLAG_INITIALIZER);
    return 0;
}

static inline int __freertos_destroy_hdl(struct KERNEL_hdl *hdl)
{
    vSemaphoreDelete(hdl);
    return 0;
}

static inline void __freertos_do_initialize(struct KERNEL_hdl *hdl)
{
    static spinlock_t atomic = SPINLOCK_INITIALIZER;

    spin_lock(&atomic);

    if (HDL_FLAG_INITIALIZER & hdl->flags)
        __freertos_init_hdl(hdl, hdl->cid, hdl->flags);

    spin_unlock(&atomic);
}

static int __freertos_acquire(struct KERNEL_hdl *hdl, uint8_t cid, uint32_t os_ticks)
{
    if (cid != hdl->cid)
        return __set_errno_neg(EINVAL);
    if ((HDL_FLAG_NO_INTR & hdl->flags) && (0 != __get_IPSR()))
        return __set_errno_neg(EACCES);
    if (HDL_FLAG_INITIALIZER & hdl->flags)
        __freertos_do_initialize(hdl);

    UBaseType_t retval;
    if (HDL_FLAG_FREERTOS_RECURSIVE & hdl->flags)
        retval = xSemaphoreTakeRecursive((void *)&hdl->padding, os_ticks);
    else
        retval = xSemaphoreTake((void *)&hdl->padding, os_ticks);

    if (pdTRUE == retval)
        return 0;
    else
        return __set_errno_neg(ETIMEDOUT);
}

static int __freertos_release(struct KERNEL_hdl *hdl, uint8_t cid)
{
    if (cid != hdl->cid)
        return __set_errno_neg(EINVAL);
    if ((HDL_FLAG_NO_INTR & hdl->flags) && (0 != __get_IPSR()))
        return __set_errno_neg(EACCES);
    if (HDL_FLAG_INITIALIZER & hdl->flags)
        __freertos_do_initialize(hdl);

    UBaseType_t retval;
    if (HDL_FLAG_FREERTOS_RECURSIVE & hdl->flags)
        retval = xSemaphoreGiveRecursive((void *)&hdl->padding);
    else
        retval = xSemaphoreGive((void *)&hdl->padding);

    if (pdTRUE == retval)
        return 0;
    else
        return __set_errno_neg(EOVERFLOW);
}

/****************************************************************************
 * @implements: mutex
*****************************************************************************/
mutex_t *mutex_create(int flags)
{
    mutex_t *mutex = KERNEL_handle_get(CID_MUTEX);
    if (mutex)
        __freertos_init_hdl(mutex, CID_MUTEX, HDL_FLAG_NO_INTR | flags | mutex->flags);

    return mutex;
}

int mutex_init(mutex_t *mutex, int flags)
{
    return __freertos_init_hdl(mutex, CID_MUTEX, HDL_FLAG_NO_INTR | flags);
}

int mutex_destroy(mutex_t *mutex)
{
    __freertos_destroy_hdl(mutex);
    return KERNEL_handle_release(mutex);
}

int mutex_lock(mutex_t *mutex)
{
    return mutex_trylock(mutex, portMAX_DELAY);
}

int mutex_trylock(mutex_t *mutex, uint32_t timeout)
{
    return __freertos_acquire(mutex, CID_MUTEX, timeout / portTICK_PERIOD_MS);
}

int mutex_unlock(mutex_t *mutex)
{
    return __freertos_release(mutex, CID_MUTEX);
}

/***************************************************************************
 *  @implements
 ***************************************************************************/
int sem_init(sem_t *sema, int pshared, unsigned int value)
{
    if (pshared)
        return __set_errno_neg(ENOSYS);
    if (SEM_VALUE_MAX < value)
        return __set_errno_neg(EINVAL);

    sema->init_sem.max_count = value;
    sema->init_sem.initial_count = 0;
    return __freertos_init_hdl(sema, CID_SEMAPHORE, 0);
}

int sem_destroy(sem_t *sema)
{
    __freertos_destroy_hdl(sema);
    return KERNEL_handle_release(sema);
}

int sem_wait(sem_t *sema)
{
    return __freertos_acquire(sema, CID_SEMAPHORE, portMAX_DELAY);
}

int sem_timedwait(sem_t *sema, struct timespec const *spec)
{
    return __freertos_acquire(sema, CID_SEMAPHORE, (spec->tv_sec * 1000 + spec->tv_nsec / 1000000) / portTICK_PERIOD_MS);
}

int sem_timedwait_ms(sem_t *sema, unsigned int millisecond)
{
    return __freertos_acquire(sema, CID_SEMAPHORE, millisecond / portTICK_PERIOD_MS);
}

int sem_post(sem_t *sema)
{
    return __freertos_release(sema, CID_SEMAPHORE);
}

int sem_getvalue(sem_t *sema, int *val)
{
    *val = uxSemaphoreGetCount(&sema->padding);
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

/****************************************************************************
 *  @implements: esp_system.h
*****************************************************************************/
esp_err_t esp_register_shutdown_handler(void (*func_ptr)(void))
{
    return atexit(func_ptr);
}

esp_err_t esp_unregister_shutdown_handler(void (*func_ptr)(void))
{
    return ESP_OK;
}

/****************************************************************************
 *  @implements: hal/systimer_hal.h
*****************************************************************************/
uint64_t systimer_ticks_to_us(uint64_t ticks)
{
    return ticks * 1000000 / CLK_systimer_freq();
}

uint64_t systimer_us_to_ticks(uint64_t us)
{
    return us * CLK_systimer_freq() / 1000000;
}

void systimer_hal_init(systimer_hal_context_t *hal)
{
    hal->dev = &SYSTIMER;
    systimer_ll_enable_clock(hal->dev, true);
}

void systimer_hal_counter_can_stall_by_cpu(systimer_hal_context_t *hal, uint32_t counter_id, uint32_t cpu_id, bool can)
{
    systimer_ll_counter_can_stall_by_cpu(hal->dev, counter_id, cpu_id, can);
}

void systimer_hal_enable_counter(systimer_hal_context_t *hal, uint32_t counter_id)
{
    systimer_ll_enable_counter(hal->dev, counter_id, true);
}

void systimer_hal_connect_alarm_counter(systimer_hal_context_t *hal, uint32_t alarm_id, uint32_t counter_id)
{
    systimer_ll_connect_alarm_counter(hal->dev, alarm_id, counter_id);
}

void systimer_hal_set_alarm_period(systimer_hal_context_t *hal, uint32_t alarm_id, uint32_t period)
{
    systimer_ll_enable_alarm(hal->dev, alarm_id, false);
    systimer_ll_set_alarm_period(hal->dev, alarm_id, hal->us_to_ticks(period));
    systimer_ll_apply_alarm_value(hal->dev, alarm_id);
    systimer_ll_enable_alarm(hal->dev, alarm_id, true);
}

void systimer_hal_select_alarm_mode(systimer_hal_context_t *hal, uint32_t alarm_id, systimer_alarm_mode_t mode)
{
    switch (mode) {
    case SYSTIMER_ALARM_MODE_ONESHOT:
        systimer_ll_enable_alarm_oneshot(hal->dev, alarm_id);
        break;
    case SYSTIMER_ALARM_MODE_PERIOD:
        systimer_ll_enable_alarm_period(hal->dev, alarm_id);
        break;
    default:
        break;
    }
}

void systimer_hal_enable_alarm_int(systimer_hal_context_t *hal, uint32_t alarm_id)
{
    systimer_ll_enable_alarm_int(hal->dev, alarm_id, true);
}

uint64_t systimer_hal_get_counter_value(systimer_hal_context_t *hal, uint32_t counter_id)
{
    uint32_t lo, lo_start, hi;
    /* Set the "update" bit and wait for acknowledgment */
    systimer_ll_counter_snapshot(hal->dev, counter_id);
    while (!systimer_ll_is_counter_value_valid(hal->dev, counter_id));
    /* Read LO, HI, then LO again, check that LO returns the same value.
     * This accounts for the case when an interrupt may happen between reading
     * HI and LO values, and this function may get called from the ISR.
     * In this case, the repeated read will return consistent values.
     */
    lo_start = systimer_ll_get_counter_value_low(hal->dev, counter_id);
    do {
        lo = lo_start;
        hi = systimer_ll_get_counter_value_high(hal->dev, counter_id);
        lo_start = systimer_ll_get_counter_value_low(hal->dev, counter_id);
    } while (lo_start != lo);

    return (uint64_t)hi << 32 | lo;
}

void systimer_hal_counter_value_advance(systimer_hal_context_t *hal, uint32_t counter_id, int64_t time_us)
{
    systimer_ll_set_counter_value(hal->dev, counter_id, systimer_hal_get_counter_value(hal, counter_id) + hal->us_to_ticks(time_us));
    systimer_ll_apply_counter_value(hal->dev, counter_id);
}
