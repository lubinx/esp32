
#include <time.h>
#include <errno.h>
#include <pthread.h>
#include <string.h>

#include <sys/errno.h>
#include <sys/mutex.h>

#include "esp_attr.h"
#include "sys/queue.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "pthread_internal.h"

/** pthread configuration structure that influences pthread creation */
typedef struct {
    size_t stack_size;  ///< The stack size of the pthread
    size_t prio;        ///< The thread's priority
    bool inherit_cfg;   ///< Inherit this configuration further
    const char *thread_name;  ///< The thread name.
    int pin_to_core;    ///< The core id to pin the thread to. Has the same value range as xCoreId argument of xTaskCreatePinnedToCore.
} esp_pthread_cfg_t;


/** task state */
enum esp_pthread_task_state
{
    PTHREAD_TASK_STATE_RUN,
    PTHREAD_TASK_STATE_EXIT
};

/** pthread thread FreeRTOS wrapper */
typedef struct esp_pthread_entry
{
    SLIST_ENTRY(esp_pthread_entry)  list_node;  ///< Tasks list node struct.
    TaskHandle_t                handle;         ///< FreeRTOS task handle
    TaskHandle_t                join_task;      ///< Handle of the task waiting to join
    enum esp_pthread_task_state state;          ///< pthread task state
    bool                        detached;       ///< True if pthread is detached
    void                       *retval;         ///< Value supplied to calling thread during join
    void                       *task_arg;       ///< Task arguments
} esp_pthread_t;

/** pthread wrapper task arg */
typedef struct
{
    void *(*func)(void *);  ///< user task entry
    void *arg;              ///< user task argument
    esp_pthread_cfg_t cfg;  ///< pthread configuration
} esp_pthread_task_arg_t;

static SemaphoreHandle_t s_threads_mux  = NULL;
portMUX_TYPE pthread_lazy_init_lock  = portMUX_INITIALIZER_UNLOCKED; // Used for mutexes and cond vars and rwlocks
static SLIST_HEAD(esp_thread_list_head, esp_pthread_entry) s_threads_list
                                        = SLIST_HEAD_INITIALIZER(s_threads_list);
static pthread_key_t s_pthread_cfg_key;


static void esp_pthread_cfg_key_destructor(void *value)
{
    free(value);
}

esp_err_t esp_pthread_init(void)
{
    if (pthread_key_create(&s_pthread_cfg_key, esp_pthread_cfg_key_destructor) != 0) {
        return ESP_ERR_NO_MEM;
    }
    s_threads_mux = xSemaphoreCreateMutex();
    if (s_threads_mux == NULL) {
        pthread_key_delete(s_pthread_cfg_key);
        return ESP_ERR_NO_MEM;
    }
    return ESP_OK;
}

static void *pthread_list_find_item(void *(*item_check)(esp_pthread_t *, void *arg), void *check_arg)
{
    esp_pthread_t *it;
    SLIST_FOREACH(it, &s_threads_list, list_node) {
        void *val = item_check(it, check_arg);
        if (val) {
            return val;
        }
    }
    return NULL;
}

static void *pthread_get_handle_by_desc(esp_pthread_t *item, void *desc)
{
    if (item == desc) {
        return item->handle;
    }
    return NULL;
}

static void *pthread_get_desc_by_handle(esp_pthread_t *item, void *hnd)
{
    if (hnd == item->handle) {
        return item;
    }
    return NULL;
}

static inline TaskHandle_t pthread_find_handle(pthread_t thread)
{
    return pthread_list_find_item(pthread_get_handle_by_desc, (void *)thread);
}

static esp_pthread_t *pthread_find(TaskHandle_t task_handle)
{
    return pthread_list_find_item(pthread_get_desc_by_handle, task_handle);
}

static void pthread_delete(esp_pthread_t *pthread)
{
    SLIST_REMOVE(&s_threads_list, pthread, esp_pthread_entry, list_node);
    free(pthread);
}

/* Call this function to configure pthread stacks in Pthreads */
esp_err_t esp_pthread_set_cfg(const esp_pthread_cfg_t *cfg)
{
    if (cfg->stack_size < CONFIG_PTHREAD_STACK_MIN) {
        return ESP_ERR_INVALID_ARG;
    }

    /* If a value is already set, update that value */
    esp_pthread_cfg_t *p = pthread_getspecific(s_pthread_cfg_key);
    if (!p) {
        p = malloc(sizeof(esp_pthread_cfg_t));
        if (!p) {
            return ESP_ERR_NO_MEM;
        }
    }
    *p = *cfg;
    pthread_setspecific(s_pthread_cfg_key, p);
    return 0;
}

esp_err_t esp_pthread_get_cfg(esp_pthread_cfg_t *p)
{
    esp_pthread_cfg_t *cfg = pthread_getspecific(s_pthread_cfg_key);
    if (cfg) {
        *p = *cfg;
        return ESP_OK;
    }
    memset(p, 0, sizeof(*p));
    return ESP_ERR_NOT_FOUND;
}

static int get_default_pthread_core(void)
{
    return CONFIG_PTHREAD_TASK_CORE_DEFAULT == -1 ? tskNO_AFFINITY : CONFIG_PTHREAD_TASK_CORE_DEFAULT;
}

esp_pthread_cfg_t esp_pthread_get_default_config(void)
{
    esp_pthread_cfg_t cfg = {
        .stack_size = CONFIG_PTHREAD_TASK_STACK_SIZE_DEFAULT,
        .prio = CONFIG_PTHREAD_TASK_PRIO_DEFAULT,
        .inherit_cfg = false,
        .thread_name = NULL,
        .pin_to_core = get_default_pthread_core()
    };

    return cfg;
}

static void pthread_task_func(void *arg)
{
    void *rval = NULL;
    esp_pthread_task_arg_t *task_arg = (esp_pthread_task_arg_t *)arg;

    // wait for start
    xTaskNotifyWait(0, 0, NULL, portMAX_DELAY);

    if (task_arg->cfg.inherit_cfg) {
        /* If inherit option is set, then do a set_cfg() ourselves for future forks,
        but first set thread_name to NULL to enable inheritance of the name too.
        (This also to prevents dangling pointers to name of tasks that might
        possibly have been deleted when we use the configuration).*/
        esp_pthread_cfg_t *cfg = &task_arg->cfg;
        cfg->thread_name = NULL;
        esp_pthread_set_cfg(cfg);
    }
    rval = task_arg->func(task_arg->arg);

    pthread_exit(rval);
}

int pthread_create2(pthread_t *thread, pthread_attr_t const *attr, pthread_routine_t routine, void *arg)
{
    struct _pthread_rtti
    {
        StaticTask_t task;
        void *exit_code;

        size_t stack_size;
        uint32_t stack[];
    };

    // struct _pthread_rtti *rtti =;


    uint32_t stack_size = attr ? attr->stack_size : CONFIG_PTHREAD_TASK_STACK_SIZE_DEFAULT;
    BaseType_t prio = CONFIG_PTHREAD_TASK_PRIO_DEFAULT;
    BaseType_t core_id = get_default_pthread_core();
    char const *task_name = CONFIG_PTHREAD_TASK_NAME_DEFAULT;
}

int pthread_create(pthread_t *thread, pthread_attr_t const *attr, pthread_routine_t routine, void *arg)
{
    TaskHandle_t xHandle = NULL;

    esp_pthread_task_arg_t *task_arg = calloc(1, sizeof(esp_pthread_task_arg_t));
    if (task_arg == NULL) {
        return ENOMEM;
    }

    esp_pthread_t *pthread = calloc(1, sizeof(esp_pthread_t));
    if (pthread == NULL) {
        free(task_arg);
        return ENOMEM;
    }

    uint32_t stack_size = CONFIG_PTHREAD_TASK_STACK_SIZE_DEFAULT;
    BaseType_t prio = CONFIG_PTHREAD_TASK_PRIO_DEFAULT;
    BaseType_t core_id = get_default_pthread_core();
    char const *task_name = CONFIG_PTHREAD_TASK_NAME_DEFAULT;

    esp_pthread_cfg_t *pthread_cfg = pthread_getspecific(s_pthread_cfg_key);
    if (pthread_cfg) {
        if (pthread_cfg->stack_size) {
            stack_size = pthread_cfg->stack_size;
        }
        if (pthread_cfg->prio && pthread_cfg->prio < configMAX_PRIORITIES) {
            prio = pthread_cfg->prio;
        }

        if (pthread_cfg->inherit_cfg) {
            if (pthread_cfg->thread_name == NULL) {
                // Inherit task name from current task.
                task_name = pcTaskGetName(NULL);
            } else {
                // Inheriting, but new task name.
                task_name = pthread_cfg->thread_name;
            }
        } else if (pthread_cfg->thread_name == NULL) {
            task_name = CONFIG_PTHREAD_TASK_NAME_DEFAULT;
        } else {
            task_name = pthread_cfg->thread_name;
        }

        if (pthread_cfg->pin_to_core >= 0 && pthread_cfg->pin_to_core < portNUM_PROCESSORS) {
            core_id = pthread_cfg->pin_to_core;
        }

        task_arg->cfg = *pthread_cfg;
    }

    if (attr) {
        /* Overwrite attributes */
        stack_size = attr->stack_size;

        switch (attr->detachstate) {
        case PTHREAD_CREATE_DETACHED:
            pthread->detached = true;
            break;
        case PTHREAD_CREATE_JOINABLE:
        default:
            pthread->detached = false;
        }
    }

    task_arg->func = routine;
    task_arg->arg = arg;
    pthread->task_arg = task_arg;
    BaseType_t res = xTaskCreatePinnedToCore(&pthread_task_func,
                                             task_name,
                                             // stack_size is in bytes. This transformation ensures that the units are
                                             // transformed to the units used in FreeRTOS.
                                             // Note: float division of ceil(m / n) ==
                                             //       integer division of (m + n - 1) / n
                                             (stack_size + sizeof(StackType_t) - 1) / sizeof(StackType_t),
                                             task_arg,
                                             prio,
                                             &xHandle,
                                             core_id);

    if (res != pdPASS) {
        free(pthread);
        free(task_arg);
        if (res == errCOULD_NOT_ALLOCATE_REQUIRED_MEMORY) {
            return ENOMEM;
        } else {
            return EAGAIN;
        }
    }
    pthread->handle = xHandle;

    if (xSemaphoreTake(s_threads_mux, portMAX_DELAY) != pdTRUE) {
        assert(false && "Failed to lock threads list!");
    }
    SLIST_INSERT_HEAD(&s_threads_list, pthread, list_node);
    xSemaphoreGive(s_threads_mux);

    // start task
    xTaskNotify(xHandle, 0, eNoAction);

    *thread = (pthread_t)pthread; // pointer value fit into pthread_t (uint32_t)

    return 0;
}

int pthread_join(pthread_t thread, void **retval)
{
    return 0;
}

int pthread_detach(pthread_t thread)
{
    return 0;
}

void pthread_exit(void *value_ptr)
{
    bool detached = false;
    /* clean up thread local storage before task deletion */
    pthread_internal_local_storage_destructor_callback(NULL);

    if (xSemaphoreTake(s_threads_mux, portMAX_DELAY) != pdTRUE) {
        assert(false && "Failed to lock threads list!");
    }
    esp_pthread_t *pthread = pthread_find(xTaskGetCurrentTaskHandle());
    if (!pthread) {
        assert(false && "Failed to find pthread for current task!");
    }
    if (pthread->task_arg) {
        free(pthread->task_arg);
    }
    if (pthread->detached) {
        // auto-free for detached threads
        pthread_delete(pthread);
        detached = true;
    } else {
        // Set return value
        pthread->retval = value_ptr;
        // Remove from list, it indicates that task has exited
        if (pthread->join_task) {
            // notify join
            xTaskNotify(pthread->join_task, 0, eNoAction);
        } else {
            pthread->state = PTHREAD_TASK_STATE_EXIT;
        }
    }

    xSemaphoreGive(s_threads_mux);
    // note: if this thread is joinable then after giving back s_threads_mux
    // this task could be deleted at any time, so don't take another lock or
    // do anything that might lock (such as printing to stdout)

    if (detached) {
        vTaskDelete(NULL);
    } else {
        vTaskSuspend(NULL);
    }

    // Should never be reached
    abort();
}

int pthread_cancel(pthread_t thread)
{
    return ENOSYS;
}

pthread_t pthread_self(void)
{
    if (xSemaphoreTake(s_threads_mux, portMAX_DELAY) != pdTRUE) {
        assert(false && "Failed to lock threads list!");
    }
    esp_pthread_t *pthread = pthread_find(xTaskGetCurrentTaskHandle());
    if (!pthread) {
        assert(false && "Failed to find current thread ID!");
    }
    xSemaphoreGive(s_threads_mux);
    return (pthread_t)pthread;
}

int pthread_equal(pthread_t t1, pthread_t t2)
{
    return ((uintptr_t)t1 - (uintptr_t)t2);
}

/***************************************************************************
 *  @implements: pthread mutex
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
    switch (detachstate) {
    case PTHREAD_CREATE_DETACHED:
        attr->detachstate = PTHREAD_CREATE_DETACHED;
        break;
    case PTHREAD_CREATE_JOINABLE:
        attr->detachstate = PTHREAD_CREATE_JOINABLE;
        break;
    default:
        return EINVAL;
    }
    return 0;
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
