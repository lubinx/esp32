#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <assert.h>
#include <errno.h>
#include <sys/reent.h>
#include <semaphore.h>

#include "spinlock.h"

#include "esp_cpu.h"
#include "esp_log.h"
#include "esp_heap_caps.h"
#include "esp_private/esp_clk.h"

#include "uart.h"

static void *blink_thread1(void *arg);
static void *blink_thread2(void *arg);
static void *blink_thread3(void *arg);


// pthread_mutex_t mutex;
static spinlock_t spinlock = SPINLOCK_INITIALIZER;
static sem_t sema;


extern "C" void __attribute__((weak)) app_main(void)
{
    esp_rom_printf("Minimum free heap size: %d bytes\n", heap_caps_get_minimum_free_size(MALLOC_CAP_DEFAULT));
    esp_rom_printf("cpu frequency: %d\n", esp_clk_cpu_freq());

    sem_init(&sema, 0, 10);
    esp_rom_printf("semaphore init...\n");

    esp_rom_printf("infinite loop...\n");


    try
    {
        throw "foobar";
    }
    catch(char const *e)
    {
        esp_rom_printf("catched c++ exception: %s\n", e);
        fflush(stdout);
    }
    printf("hello world!\n");

    int fd = UART_createfd(0, 115200, paNone, 1);
    (void)fd;

    FILE *f = fopen("foo", "w+");
    if (f)
    {
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
        sem_post(&sema);

        spinlock_acquire(&spinlock, SPINLOCK_WAIT_FOREVER);
        esp_rom_printf("thread/cpu (0 <=> %d)\n", esp_cpu_get_core_id());
        fflush(stdout);
        spinlock_release(&spinlock);

        msleep(500);
    }
}

static void *blink_thread1(void *arg)
{
    ARG_UNUSED(arg);

    // uint32_t irq_status = XTOS_SET_INTLEVEL(XCHAL_EXCM_LEVEL);
    // esp_rom_printf("...............................%lu\n\n\n", irq_status);

    while (true)
    {
        /*
        for (int i = 0; i < 1000; i ++)
            esp_rom_delay_us(1000);

        esp_rom_printf("thread1: cpu: %d, val %d\n", esp_cpu_get_core_id(), val);

        spinlock_acquire(&spinlock, SPINLOCK_WAIT_FOREVER);
        */
        spinlock_acquire(&spinlock, SPINLOCK_WAIT_FOREVER);
        esp_rom_printf("thread/cpu (1 <=> %d)\n", esp_cpu_get_core_id());

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
        esp_rom_printf("thread/cpu (2 <=> %d)\n", esp_cpu_get_core_id());
        fflush(stdout);
        spinlock_release(&spinlock);

        sem_timedwait_ms(&sema, 600);
    }
}

static void *blink_thread3(void *arg)
{
    ARG_UNUSED(arg);

    while (true)
    {
        spinlock_acquire(&spinlock, SPINLOCK_WAIT_FOREVER);
        esp_rom_printf("thread/cpu (3 <=> %d)\n", esp_cpu_get_core_id());
        fflush(stdout);
        spinlock_release(&spinlock);

        msleep(800);
    }
}
