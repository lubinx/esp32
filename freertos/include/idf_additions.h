/*
 * SPDX-FileCopyrightText: 2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
// #include "idf_additions_inc.h"

#if CONFIG_FREERTOS_SMP || __DOXYGEN__
/**
 * @brief Get the handle of the task running on a certain core
 *
 * Because of the nature of SMP processing, there is no guarantee that this value will still be valid on return and
 * should only be used for debugging purposes.
 *
 * [refactor-todo] Mark this function as deprecated, call xTaskGetCurrentTaskHandleCPU() instead
 *
 * @param xCoreID The core to query
 * @return Handle of the current task running on the queried core
 */
TaskHandle_t xTaskGetCurrentTaskHandleForCPU( BaseType_t xCoreID );

/**
 * @brief Get the handle of idle task for the given CPU.
 *
 * [refactor-todo] Mark this function as deprecated, call xTaskGetIdleTaskHandle() instead
 *
 * @param xCoreID The core to query
 * @return Handle of the idle task for the queried core
 */
TaskHandle_t xTaskGetIdleTaskHandleForCPU( BaseType_t xCoreID );

/**
 * @brief Get the current core affintiy of a particular task
 *
 * Helper function to get the core affinity of a particular task. If the task is pinned to a particular core, the core
 * ID is returned. If the task is not pinned to a particular core, tskNO_AFFINITY is returned.
 *
 * [refactor-todo] Mark this function as deprecated, call vTaskCoreAffinityGet() instead
 *
 * @param xTask The task to query
 * @return The tasks coreID or tskNO_AFFINITY
 */
BaseType_t xTaskGetAffinity( TaskHandle_t xTask );

#if ( CONFIG_FREERTOS_TLSP_DELETION_CALLBACKS )

    /**
     * Prototype of local storage pointer deletion callback.
     */
    typedef void (*TlsDeleteCallbackFunction_t)( int, void * );

    /**
     * Set local storage pointer and deletion callback.
     *
     * Each task contains an array of pointers that is dimensioned by the
     * configNUM_THREAD_LOCAL_STORAGE_POINTERS setting in FreeRTOSConfig.h.
     * The kernel does not use the pointers itself, so the application writer
     * can use the pointers for any purpose they wish.
     *
     * Local storage pointers set for a task can reference dynamically
     * allocated resources. This function is similar to
     * vTaskSetThreadLocalStoragePointer, but provides a way to release
     * these resources when the task gets deleted. For each pointer,
     * a callback function can be set. This function will be called
     * when task is deleted, with the local storage pointer index
     * and value as arguments.
     *
     * @param xTaskToSet  Task to set thread local storage pointer for
     * @param xIndex The index of the pointer to set, from 0 to
     *               configNUM_THREAD_LOCAL_STORAGE_POINTERS - 1.
     * @param pvValue  Pointer value to set.
     * @param pvDelCallback  Function to call to dispose of the local
     *                       storage pointer when the task is deleted.
     */
    void vTaskSetThreadLocalStoragePointerAndDelCallback(
            TaskHandle_t xTaskToSet,
            BaseType_t xIndex,
            void *pvValue,
            TlsDeleteCallbackFunction_t pvDelCallback);
#endif // CONFIG_FREERTOS_TLSP_DELETION_CALLBACKS

#endif // CONFIG_FREERTOS_SMP || __DOXYGEN__
