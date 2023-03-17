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
    TCB_t *pxCurTask = xTaskGetCurrentTaskHandle();

    if (pxCurTask)
        return &pxCurTask->xNewLib_reent;
    else
        return _GLOBAL_REENT;
}
