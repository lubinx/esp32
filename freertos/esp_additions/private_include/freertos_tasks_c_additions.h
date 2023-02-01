/*
 * SPDX-FileCopyrightText: 2015-2021 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "sdkconfig.h"

/**
 * This file will be included in `tasks.c` file, thus, it must NOT be included
 * by any (other) file.
 * The functions below only consist in getters for the static variables in
 * `tasks.c` file.
 * The only source files that should call these functions are the ones in
 * `/additions` directory.
 */

/* ----------------------------------------------------- Newlib --------------------------------------------------------
 *
 * ------------------------------------------------------------------------------------------------------------------ */

#if ( configUSE_NEWLIB_REENTRANT == 1 )
/**
 * @brief Get reentrancy structure of the current task
 *
 * - This funciton is required by newlib (when __DYNAMIC_REENT__ is enabled)
 * - It will return a pointer to the current task's reent struct
 * - If FreeRTOS is not running, it will return the global reent struct
 *
 * @return Pointer to a the (current taks's)/(globa) reent struct
 */
struct _reent *__getreent(void)
{
    // No lock needed because if this changes, we won't be running anymore.
    TCB_t *pxCurTask = xTaskGetCurrentTaskHandle();
    struct _reent *ret;
    if (pxCurTask == NULL) {
        // No task running. Return global struct.
        ret = _GLOBAL_REENT;
    } else {
        // We have a task; return its reentrant struct.
        ret = &pxCurTask->xNewLib_reent;
    }
    return ret;
}
#endif // configUSE_NEWLIB_REENTRANT == 1

/* ----------------------------------------------------- OpenOCD -------------------------------------------------------
 *
 * ------------------------------------------------------------------------------------------------------------------ */

#if ( configENABLE_FREERTOS_DEBUG_OCDAWARE == 1 )

/**
 * Debug param indexes. DO NOT change the order. OpenOCD uses the same indexes
 * Entries in FreeRTOS_openocd_params must match the order of these indexes
 */
enum {
    ESP_FREERTOS_DEBUG_TABLE_SIZE = 0,
    ESP_FREERTOS_DEBUG_TABLE_VERSION,
    ESP_FREERTOS_DEBUG_KERNEL_VER_MAJOR,
    ESP_FREERTOS_DEBUG_KERNEL_VER_MINOR,
    ESP_FREERTOS_DEBUG_KERNEL_VER_BUILD,
    ESP_FREERTOS_DEBUG_UX_TOP_USED_PIORITY,
    ESP_FREERTOS_DEBUG_PX_TOP_OF_STACK,
    ESP_FREERTOS_DEBUG_PC_TASK_NAME,
    /* New entries must be inserted here */
    ESP_FREERTOS_DEBUG_TABLE_END,
};

const DRAM_ATTR uint8_t FreeRTOS_openocd_params[ESP_FREERTOS_DEBUG_TABLE_END]  = {
    ESP_FREERTOS_DEBUG_TABLE_END,       /* table size */
    1,                                  /* table version */
    tskKERNEL_VERSION_MAJOR,
    tskKERNEL_VERSION_MINOR,
    tskKERNEL_VERSION_BUILD,
    configMAX_PRIORITIES - 1,           /* uxTopUsedPriority */
    offsetof(TCB_t, pxTopOfStack),      /* thread_stack_offset; */
    offsetof(TCB_t, pcTaskName),        /* thread_name_offset; */
};

#endif // configENABLE_FREERTOS_DEBUG_OCDAWARE == 1

/* -------------------------------------------- FreeRTOS IDF API Additions ---------------------------------------------
 * FreeRTOS related API that were added by IDF
 * ------------------------------------------------------------------------------------------------------------------ */

#if CONFIG_FREERTOS_SMP
_Static_assert(tskNO_AFFINITY == CONFIG_FREERTOS_NO_AFFINITY, "CONFIG_FREERTOS_NO_AFFINITY must be the same as tskNO_AFFINITY");

BaseType_t xTaskCreatePinnedToCore( TaskFunction_t pxTaskCode,
                                    const char *const pcName,
                                    uint32_t const usStackDepth,
                                    void *const pvParameters,
                                    UBaseType_t uxPriority,
                                    TaskHandle_t *const pxCreatedTask,
                                    const BaseType_t xCoreID)
{
    BaseType_t ret;
    #if ( ( configUSE_CORE_AFFINITY == 1 ) && ( configNUM_CORES > 1 ) )
        {
            // Convert xCoreID into an affinity mask
            UBaseType_t uxCoreAffinityMask;
            if (xCoreID == tskNO_AFFINITY) {
                uxCoreAffinityMask = tskNO_AFFINITY;
            } else {
                uxCoreAffinityMask = (1 << xCoreID);
            }
            ret = xTaskCreateAffinitySet(pxTaskCode, pcName, usStackDepth, pvParameters, uxPriority, uxCoreAffinityMask, pxCreatedTask);
        }
    #else /* ( ( configUSE_CORE_AFFINITY == 1 ) && ( configNUM_CORES > 1 ) ) */
        {
            ret = xTaskCreate(pxTaskCode, pcName, usStackDepth, pvParameters, uxPriority, pxCreatedTask);
        }
    #endif /* ( ( configUSE_CORE_AFFINITY == 1 ) && ( configNUM_CORES > 1 ) ) */
    return ret;
}

#if ( configSUPPORT_STATIC_ALLOCATION == 1 )
TaskHandle_t xTaskCreateStaticPinnedToCore( TaskFunction_t pxTaskCode,
                                            const char *const pcName,
                                            uint32_t const ulStackDepth,
                                            void *const pvParameters,
                                            UBaseType_t uxPriority,
                                            StackType_t *const puxStackBuffer,
                                            StaticTask_t *const pxTaskBuffer,
                                            const BaseType_t xCoreID)
{
    TaskHandle_t ret;
    #if ( ( configUSE_CORE_AFFINITY == 1 ) && ( configNUM_CORES > 1 ) )
        {
            // Convert xCoreID into an affinity mask
            UBaseType_t uxCoreAffinityMask;
            if (xCoreID == tskNO_AFFINITY) {
                uxCoreAffinityMask = tskNO_AFFINITY;
            } else {
                uxCoreAffinityMask = (1 << xCoreID);
            }
            ret = xTaskCreateStaticAffinitySet(pxTaskCode, pcName, ulStackDepth, pvParameters, uxPriority, puxStackBuffer, pxTaskBuffer, uxCoreAffinityMask);
        }
    #else /* ( ( configUSE_CORE_AFFINITY == 1 ) && ( configNUM_CORES > 1 ) ) */
        {
            ret = xTaskCreateStatic(pxTaskCode, pcName, ulStackDepth, pvParameters, uxPriority, puxStackBuffer, pxTaskBuffer);
        }
    #endif /* ( ( configUSE_CORE_AFFINITY == 1 ) && ( configNUM_CORES > 1 ) ) */
    return ret;
}
#endif /* configSUPPORT_STATIC_ALLOCATION */

TaskHandle_t xTaskGetCurrentTaskHandleForCPU( BaseType_t xCoreID )
{
    TaskHandle_t xTaskHandleTemp;
    assert(xCoreID >= 0 && xCoreID < configNUM_CORES);
    taskENTER_CRITICAL();
    xTaskHandleTemp = (TaskHandle_t) pxCurrentTCBs[xCoreID];
    taskEXIT_CRITICAL();
    return xTaskHandleTemp;
}

TaskHandle_t xTaskGetIdleTaskHandleForCPU( BaseType_t xCoreID )
{
    assert(xCoreID >= 0 && xCoreID < configNUM_CORES);
    return (TaskHandle_t) xIdleTaskHandle[xCoreID];
}

BaseType_t xTaskGetAffinity( TaskHandle_t xTask )
{
    taskENTER_CRITICAL();
    UBaseType_t uxCoreAffinityMask;
#if ( configUSE_CORE_AFFINITY == 1 && configNUM_CORES > 1 )
    TCB_t *pxTCB = prvGetTCBFromHandle( xTask );
    uxCoreAffinityMask = pxTCB->uxCoreAffinityMask;
#else
    uxCoreAffinityMask = tskNO_AFFINITY;
#endif
    taskEXIT_CRITICAL();
    BaseType_t ret;
    // If the task is not pinned to a particular core, treat it as tskNO_AFFINITY
    if (uxCoreAffinityMask & (uxCoreAffinityMask - 1)) {    // If more than one bit set
        ret = tskNO_AFFINITY;
    } else {
        int index_plus_one = __builtin_ffs(uxCoreAffinityMask);
        assert(index_plus_one >= 1);
        ret = index_plus_one - 1;
    }
    return ret;
}

#if ( CONFIG_FREERTOS_TLSP_DELETION_CALLBACKS )
void vTaskSetThreadLocalStoragePointerAndDelCallback( TaskHandle_t xTaskToSet, BaseType_t xIndex, void *pvValue, TlsDeleteCallbackFunction_t pvDelCallback)
    {
        // Verify that the offsets of pvThreadLocalStoragePointers and pvDummy15 match.
        // pvDummy15 is part of the StaticTask_t struct and is used to access the TLSPs
        // while deletion.
        _Static_assert(offsetof( StaticTask_t, pvDummy15 ) == offsetof( TCB_t, pvThreadLocalStoragePointers ), "Offset of pvDummy15 must match the offset of pvThreadLocalStoragePointers");

        //Set the local storage pointer first
        vTaskSetThreadLocalStoragePointer( xTaskToSet, xIndex, pvValue );

        //Set the deletion callback at an offset of configNUM_THREAD_LOCAL_STORAGE_POINTERS/2
        vTaskSetThreadLocalStoragePointer( xTaskToSet, ( xIndex + ( configNUM_THREAD_LOCAL_STORAGE_POINTERS / 2 ) ), pvDelCallback );
    }
#endif // CONFIG_FREERTOS_TLSP_DELETION_CALLBACKS

#endif // CONFIG_FREERTOS_SMP
