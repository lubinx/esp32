// dummy
/*
    esp freertos porting by calling
        void esp_vApplicationTickHook(void)
            for each systick ticks

        void esp_vApplicationIdleHook(void)
            when no task to run

    implemented *weak* in esp_system/esp_freertos_impl.c

    these should be simple logic, deprecate old implement register/unregister etc.
    futher override should by user code, not system code
*/
