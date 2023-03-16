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

#endif // CONFIG_FREERTOS_SMP || __DOXYGEN__
