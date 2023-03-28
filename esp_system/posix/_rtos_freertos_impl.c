/*
 * SPDX-FileCopyrightText: 2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stddef.h>
#include <assert.h>

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/semphr.h>

#include <clk-tree.h>

#include <esp_attr.h>
#include <esp_log.h>
#include <esp_heap_caps.h>

#include "rtos/kernel.h"
#include "sys/mutex.h"
#include "semaphore.h"
#include "esp_system.h"

#include "hal/systimer_ll.h"
#include "hal/systimer_hal.h"

#include "sdkconfig.h"

static char const *TAG = "freertos";
static_assert(configSUPPORT_STATIC_ALLOCATION, "FreeRTOS should be configured with static allocation support");

/****************************************************************************
 *  @def
*****************************************************************************/
struct __freertos_tcb
{
    struct KERNEL_tcb kernel;

//--- thread freertos parameters
    unsigned priority;
    bool task_is_dynamic_alloc;
    bool stack_is_dynamic_alloc;
    // __freertos_tcb <==> __freertos_task
    struct __freertos_task *task_ptr;
};

struct __freertos_task
{
    StaticTask_t _sinit;
    // __freertos_tcb <==> __freertos_task
    struct __freertos_tcb *tcb;
};

struct freertos_task_pool
{
    spinlock_t atomic;
    glist_t freed;
    struct __freertos_task tasks[3];
};

/// @internal
static struct freertos_task_pool task_pool = {.atomic = SPINLOCK_INITIALIZER};
static char const *__freertos_argv = "freertos_start";

/****************************************************************************
 *  @implements: freertos tick & idle
*****************************************************************************/
__attribute__((weak))
void IRAM_ATTR vApplicationTickHook(void)
{
    // nothing to do
}

void IRAM_ATTR vApplicationIdleHook(void)
{
    KERNEL_handle_recycle();
    __WFI();
}

/****************************************************************************
 *  @implements: freertos main thread
*****************************************************************************/
static void __freertos_start(void *arg)
{
    ARG_UNUSED(arg);
    __esp_rtos_initialize();

    extern __attribute__((noreturn)) int main(int argc, char **argv);
    // TODO: process main() exit code
    main(1, (char **)&__freertos_argv);

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
        #if defined(CONFIG_ESP_MAIN_TASK_AFFINITY_NO_AFFINITY) && CONFIG_ESP_MAIN_TASK_AFFINITY_NO_AFFINITY
            xTaskCreateStatic(__freertos_start, __freertos_argv,
                CONFIG_ESP_MAIN_TASK_STACK_SIZE, NULL, configMAX_PRIORITIES,
                (void *)__main_stack, &__main_task
            );
        #else
            xTaskCreateStaticAffinitySet(__freertos_start, __freertos_argv,
                CONFIG_ESP_MAIN_TASK_STACK_SIZE, NULL, configMAX_PRIORITIES,
                (void *)__main_stack, &__main_task, CONFIG_ESP_MAIN_TASK_AFFINITY
            );
        #endif

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
 *  @implements: freertos threads
*****************************************************************************/
static void __freertos_thread_entry(struct __freertos_tcb *tcb)
{
    tcb->kernel.exit_code = tcb->kernel.start_routine(tcb->kernel.arg);
    struct __freertos_task *task = tcb->task_ptr;

    if (! tcb->task_is_dynamic_alloc)
    {
        spin_lock(&task_pool.atomic);
        glist_push_back(&task_pool.freed, task);
        spin_unlock(&task_pool.atomic);
    }
    else
        KERNEL_mfree(task);

    if (tcb->stack_is_dynamic_alloc)
        KERNEL_mfree(tcb->kernel.stack_base);

    vTaskDelete((void *)&task->_sinit);
    KERNEL_handle_release(tcb);
}

thread_id_t thread_create(unsigned priority, void *(*start_rountine)(void *arg), void *arg,
    uint32_t *stack, size_t stack_size)
{
    return thread_create_at_core(priority, start_rountine, arg,
        stack, stack_size, THREAD_BIND_ALL_CORE);
}

thread_id_t thread_create_at_core(unsigned priority, void *(*start_rountine)(void *arg), void *arg,
    uint32_t *stack, size_t stack_size, unsigned core_id)
{
    if (stack_size < THREAD_MINIMAL_STACK_SIZE)
        return __set_errno_nullptr(EINVAL);
    if (THREAD_BIND_ALL_CORE != core_id && SOC_CPU_CORES_NUM - 1 < core_id)
        return __set_errno_nullptr(EINVAL);

    void *dynamic_stack = NULL;
    if (! stack)
    {
        dynamic_stack = KERNEL_malloc(stack_size);

        if (! dynamic_stack)
            return __set_errno_nullptr(ENOMEM);
    }

    spin_lock(&task_pool.atomic);
    if (! glist_is_initialized(&task_pool.freed))
    {
        glist_initialize(&task_pool.freed);

        for (unsigned i = 0; i < lengthof(task_pool.tasks); i ++)
            glist_push_back(&task_pool.freed, &task_pool.tasks[i]);
    }
    struct __freertos_task *task = glist_pop(&task_pool.freed);
    spin_unlock(&task_pool.atomic);

    struct __freertos_task *dynamic_task = NULL;
    if (! task)
    {
        dynamic_task = KERNEL_malloc(sizeof(struct __freertos_task));

        if (! dynamic_task)
            return __set_errno_nullptr(ENOMEM);
    }

    struct __freertos_tcb *tcb = KERNEL_handle_get(CID_TCB);
    if (tcb)
    {
        tcb->kernel.start_routine = start_rountine;
        tcb->kernel.arg = arg;
        tcb->kernel.stack_size = stack_size;
        tcb->priority = priority;

        if (dynamic_stack)
        {
            tcb->kernel.stack_base = dynamic_stack;
            tcb->stack_is_dynamic_alloc |= HDL_FLAG_SYSMEM_MANAGED;
        }
        else
            tcb->kernel.stack_base = stack;

        if (dynamic_task)
        {
            task = dynamic_task;
            tcb->task_is_dynamic_alloc = true;
        }

        TaskHandle_t hdl;
        if (THREAD_BIND_ALL_CORE == core_id)
        {
            hdl = xTaskCreateStatic((void *)__freertos_thread_entry, NULL,
                stack_size, tcb, priority,
                tcb->kernel.stack_base, &task->_sinit
            );
        }
        else
        {
            hdl = xTaskCreateStaticAffinitySet((void *)__freertos_thread_entry, NULL,
                stack_size, tcb, priority,
                tcb->kernel.stack_base, &task->_sinit, 1 << core_id
            );
        }

        // NOTE: freertos xTaskCreateStatic() task should equal to task itself, this is the feature *MUST*
        if (hdl != (void *)task)
        {
            ESP_LOGE(TAG, "freertos xTaskCreateStatic() task *MUST* equal to task itself, this is the feature we *REQUIRED*");
            while (1);
        }
        /// circle ref: tcb->task_ptr == task->tcb
        tcb->task_ptr = task;
        task->tcb = tcb;
    }
    else
    {
        if (task)
        {
            spin_lock(&task_pool.atomic);
            glist_push_back(&task_pool.freed, task);
            spin_unlock(&task_pool.atomic);
        }
        else if (dynamic_task)
            KERNEL_mfree(task);

        if (dynamic_stack)
            KERNEL_mfree(dynamic_stack);
    }
    return tcb;
}

thread_id_t thread_self(void)
{
    return ((struct __freertos_task *)xTaskGetCurrentTaskHandle())->tcb;
}

int thread_join(thread_id_t thread)
{
    ARG_UNUSED(thread);
    return 0;
}

int thread_detach(thread_id_t thread)
{
    ARG_UNUSED(thread);
    return 0;
}

/****************************************************************************
 *  @implements: unistd.h sleep() / msleep() / usleep(), sched.h sched_yield()
*****************************************************************************/
int sched_yield(void)
{
    taskYIELD();
    return 0;
}

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

    unsigned tick_start = __get_CCOUNT();
    unsigned ticks = us * (unsigned)(CLK_cpu_freq() / _MHZ);

    while ((unsigned)(__get_CCOUNT() - tick_start) < ticks) {}
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

/****************************************************************************
 * @implements: generic waitfor
*****************************************************************************/
int waitfor(handle_t hdl, uint32_t timeout)
{
    UBaseType_t retval;
    if (HDL_FLAG_RECURSIVE_MUTEX & AsKernelHdl(hdl)->flags)
        retval = xSemaphoreTakeRecursive((void *)&AsKernelHdl(hdl)->padding, timeout / portTICK_PERIOD_MS);
    else
        retval = xSemaphoreTake((void *)&AsKernelHdl(hdl)->padding, timeout / portTICK_PERIOD_MS);

    if (pdTRUE == retval)
        return 0;
    else
        return __set_errno_neg(ETIMEDOUT);
}

/****************************************************************************
 * @internal: generic synchronize objects
*****************************************************************************/
static int __freertos_hdl_init(struct KERNEL_hdl *hdl, uint8_t cid, uint8_t flags)
{
    switch (cid)
    {
    case CID_SEMAPHORE:
        xSemaphoreCreateCountingStatic(hdl->init_sem.max_count, hdl->init_sem.initial_count, (void *)hdl->padding);
        break;

    case CID_MUTEX:
        if (HDL_FLAG_RECURSIVE_MUTEX & flags)
            xSemaphoreCreateRecursiveMutexStatic((void *)hdl->padding);
        else
            xSemaphoreCreateMutexStatic((void *)&hdl->padding);
        break;

    default:
        return ENOSYS;
    }

    hdl->cid = cid;
    hdl->flags = flags & (uint8_t)(~(HDL_FLAG_INITIALIZER));
    return 0;
}

static int __freertos_hdl_destroy(struct KERNEL_hdl *hdl)
{
    vSemaphoreDelete(hdl);
    return 0;
}

static void __freertos_hdl_initializer(struct KERNEL_hdl *hdl)
{
    static spinlock_t atomic = SPINLOCK_INITIALIZER;
    spin_lock(&atomic);

    if (HDL_FLAG_INITIALIZER & hdl->flags)
        __freertos_hdl_init(hdl, hdl->cid, hdl->flags);

    spin_unlock(&atomic);
}

static IRAM_ATTR int __freertos_hdl_acquire(struct KERNEL_hdl *hdl, uint8_t cid, uint32_t os_ticks)
{
    if (cid != hdl->cid)
        return EINVAL;
    if ((HDL_FLAG_NO_INTR & hdl->flags) && (0 != __get_IPSR()))
        return EACCES;
    if (HDL_FLAG_INITIALIZER & hdl->flags)
        __freertos_hdl_initializer(hdl);

    UBaseType_t retval;
    if (HDL_FLAG_RECURSIVE_MUTEX & hdl->flags)
        retval = xSemaphoreTakeRecursive((void *)&hdl->padding, os_ticks);
    else
        retval = xSemaphoreTake((void *)&hdl->padding, os_ticks);

    if (pdTRUE == retval)
        return 0;
    else
        return ETIMEDOUT;
}

static IRAM_ATTR int __freertos_hdl_release(struct KERNEL_hdl *hdl, uint8_t cid)
{
    if (cid != hdl->cid)
        return EINVAL;
    if ((HDL_FLAG_NO_INTR & hdl->flags) && (0 != __get_IPSR()))
        return EACCES;
    if (HDL_FLAG_INITIALIZER & hdl->flags)
        __freertos_hdl_initializer(hdl);

    UBaseType_t retval;
    if (HDL_FLAG_RECURSIVE_MUTEX & hdl->flags)
        retval = xSemaphoreGiveRecursive((void *)&hdl->padding);
    else
        retval = xSemaphoreGive((void *)&hdl->padding);

    if (pdTRUE == retval)
        return 0;
    else
        return EOVERFLOW;
}

/****************************************************************************
 * @implements: sys/mutex.h
*****************************************************************************/
mutex_t *mutex_create(int flags)
{
    mutex_t *mutex = KERNEL_handle_get(CID_MUTEX);
    if (mutex)
        __freertos_hdl_init(mutex, CID_MUTEX, HDL_FLAG_NO_INTR | (uint8_t)flags | mutex->flags);

    return mutex;
}

int mutex_init(mutex_t *mutex, int flags)
{
    return __freertos_hdl_init(mutex, CID_MUTEX, HDL_FLAG_NO_INTR | (uint8_t)flags);
}

int mutex_destroy(mutex_t *mutex)
{
    __freertos_hdl_destroy(mutex);
    return KERNEL_handle_release(mutex);
}

int IRAM_ATTR mutex_lock(mutex_t *mutex)
{
    return mutex_trylock(mutex, portMAX_DELAY);
}

int IRAM_ATTR mutex_trylock(mutex_t *mutex, uint32_t timeout)
{
    return __freertos_hdl_acquire(mutex, CID_MUTEX, timeout / portTICK_PERIOD_MS);
}

int IRAM_ATTR mutex_unlock(mutex_t *mutex)
{
    return __freertos_hdl_release(mutex, CID_MUTEX);
}

/***************************************************************************
 *  @implements: semaphore.h
 ***************************************************************************/
int sem_init(sem_t *sema, int pshared, unsigned int value)
{
    return sem_init_np(sema, pshared, value, SEM_VALUE_MAX);
}

int sem_init_np(sem_t *sema, int pshared, unsigned int value, unsigned int max)
{
    if (pshared)
        return __set_errno_neg(ENOSYS);
    if (SEM_VALUE_MAX < value)
        return __set_errno_neg(EINVAL);

    sema->init_sem.max_count = max;
    sema->init_sem.initial_count = value;

    int retval = __freertos_hdl_init(sema, CID_SEMAPHORE, 0);

    if (retval)
        return __set_errno_neg(retval);
    else
        return retval;
}

int sem_destroy(sem_t *sema)
{
    __freertos_hdl_destroy(sema);
    int retval = KERNEL_handle_release(sema);

    if (retval)
        return __set_errno_neg(retval);
    else
        return retval;
}

int IRAM_ATTR sem_wait(sem_t *sema)
{
    int retval = __freertos_hdl_acquire(sema, CID_SEMAPHORE, portMAX_DELAY);

    if (retval)
        return __set_errno_neg(retval);
    else
        return retval;
}

int IRAM_ATTR sem_timedwait(sem_t *sema, struct timespec const *abs_timeout)
{
    int retval = __freertos_hdl_acquire(sema, CID_SEMAPHORE,
        (uint32_t)((abs_timeout->tv_sec * 1000 + abs_timeout->tv_nsec / 1000000) / portTICK_PERIOD_MS));

    if (retval)
        return __set_errno_neg(retval);
    else
        return retval;
}

int IRAM_ATTR sem_timedwait_ms(sem_t *sema, unsigned int millisecond)
{
    int retval = __freertos_hdl_acquire(sema, CID_SEMAPHORE, millisecond / portTICK_PERIOD_MS);

    if (retval)
        return __set_errno_neg(retval);
    else
        return retval;
}

int IRAM_ATTR sem_post(sem_t *sema)
{
    int retval = __freertos_hdl_release(sema, CID_SEMAPHORE);

    if (retval)
        return __set_errno_neg(retval);
    else
        return retval;
}

int IRAM_ATTR sem_getvalue(sem_t *sema, int *val)
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
    ARG_UNUSED(func_ptr);
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
    systimer_ll_set_alarm_period(hal->dev, alarm_id, (uint32_t)hal->us_to_ticks(period));
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
    systimer_ll_set_counter_value(hal->dev, counter_id,
        systimer_hal_get_counter_value(hal, counter_id) +
        hal->us_to_ticks((uint64_t)time_us)
    );
    systimer_ll_apply_counter_value(hal->dev, counter_id);
}
