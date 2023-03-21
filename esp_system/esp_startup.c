#include <esp_system.h>
#include <soc.h>

#include "sdkconfig.h"

static char const *TAG = "startup";

/****************************************************************************
 *  imports
*****************************************************************************/
extern uint32_t __zero_table_start__;
extern uint32_t __zero_table_end__;

// gcc ctors
extern void (*__init_array_start)(void);
extern void (*__init_array_end)(void);

/****************************************************************************
 *  exports
*****************************************************************************/
void Startup_Handler(void)
{
    // bootloader already loaded these
    /*
    struct copy_table_t
    {
        uint32_t const *src;
        uint32_t *dst;
        uint32_t  size;
    };

    for (struct copy_table_t const *tbl = (struct copy_table_t const *)&__copy_table_start__;
        tbl < (struct copy_table_t const *)&__copy_table_end__;
        tbl ++)
    {
        for(uint32_t i = 0; i < tbl->size / sizeof(uint32_t); i ++)
            tbl->dst[i] = tbl->src[i];
    }
    */

    struct zero_table_t
    {
        uint32_t *dst;
        uint32_t size;
    };

    for (struct zero_table_t const *tbl = (struct zero_table_t const *)&__zero_table_start__;
        tbl < (struct zero_table_t const *)&__zero_table_end__;
        tbl ++)
    {
        for(uint32_t i = 0; i < tbl->size / sizeof(*tbl->dst); i ++)
            tbl->dst[i] = 0;
    }

    /*
    for (uint64_t i = 0; i < 5000000; i ++)
        (void)__get_CCOUNT();
    */

    SOC_initialize();

    extern void __libc_retarget_init(void); //  _retarget_init.c
    __libc_retarget_init();

    #ifdef CONFIG_COMPILER_CXX_EXCEPTIONS
        struct object { long placeholder[10]; };
        extern void __register_frame_info(const void *begin, struct object *ob);
        extern char __eh_frame[];

        static struct object ob;
        __register_frame_info(__eh_frame, &ob);
    #endif

    // gcc ctors
    for (void (**p)(void) = &__init_array_start;
        p < &__init_array_end;
        p ++)
    {
        (*p)();
    }

    esp_rtos_bootstrap();
}
