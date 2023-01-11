#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <assert.h>
#include <errno.h>
#include "spinlock.h"

#include "freertos/FreeRTOS.h"

#include "esp_cpu.h"
#include "esp_log.h"
#include "esp_heap_caps.h"
#include "esp_private/esp_clk.h"

static void *blink_thread1(void *arg);
static void *blink_thread2(void *arg);
static void *blink_thread3(void *arg);

// pthread_mutex_t mutex;
spinlock_t spinlock = SPINLOCK_INITIALIZER;

extern "C" void __attribute__((weak)) app_main(void)
{
    printf("Minimum free heap size: %d bytes\n", heap_caps_get_minimum_free_size(MALLOC_CAP_DEFAULT));
    printf("cpu frequency: %d\n", esp_clk_cpu_freq());

    printf("infinite loop...\n");

    try
    {
        throw "foobar";
    }
    catch(char const *e)
    {
        printf("catched c++ exception: %s\n", e);
        fflush(stdout);
    }

    /*
    pthread_mutexattr_t attr;
    pthread_mutexattr_init(&attr);
    pthread_mutex_init(&mutex, &attr);
    */

    pthread_t id;
    pthread_create(&id, NULL, blink_thread1, NULL);
    pthread_create(&id, NULL, blink_thread2, NULL);
    pthread_create(&id, NULL, blink_thread3, NULL);

    while (1)
    {
        spinlock_acquire(&spinlock, SPINLOCK_WAIT_FOREVER);
        esp_rom_printf("...............................%d\n", esp_cpu_get_core_id());
        fflush(stdout);
        spinlock_release(&spinlock);

        msleep(500);
    }
}

static void *blink_thread1(void *arg)
{
    ARG_UNUSED(arg);
    int start_core_id = esp_cpu_get_core_id();

    // uint32_t irq_status = XTOS_SET_INTLEVEL(XCHAL_EXCM_LEVEL);
    // esp_rom_printf("...............................%lu\n\n\n", irq_status);

    while (true)
    {
        /*
        for (int i = 0; i < 1000; i ++)
            esp_rom_delay_us(1000);

        printf("thread1: cpu: %d, val %d\n", esp_cpu_get_core_id(), val);

        spinlock_acquire(&spinlock, SPINLOCK_WAIT_FOREVER);
        */
        spinlock_acquire(&spinlock, SPINLOCK_WAIT_FOREVER);
        int core_id = esp_cpu_get_core_id();

        if (start_core_id == core_id)
            printf("thread1: cpu: %d start_core_id == core_id\n", core_id);
        else
            printf("thread1: cpu: %d ########### start_core_id != core_id\n", core_id);

        fflush(stdout);
        spinlock_release(&spinlock);

        msleep(600);
        /*
        for (int i = 0; i < 1000; i ++)
            esp_rom_delay_us(800);
        */
    }

}

static void *blink_thread2(void *arg)
{
    ARG_UNUSED(arg);

    while (true)
    {
        spinlock_acquire(&spinlock, SPINLOCK_WAIT_FOREVER);
        printf("thread2: cpu: %d\n", esp_cpu_get_core_id());
        fflush(stdout);
        spinlock_release(&spinlock);

        msleep(700);
    }
}

static void *blink_thread3(void *arg)
{
    ARG_UNUSED(arg);

    while (true)
    {
        spinlock_acquire(&spinlock, SPINLOCK_WAIT_FOREVER);
        printf("thread3: cpu: %d\n", esp_cpu_get_core_id());
        fflush(stdout);
        spinlock_release(&spinlock);

        msleep(800);
    }
}
