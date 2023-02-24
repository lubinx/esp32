#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <semaphore.h>

#include "clk_tree.h"
#include "gpio.h"
#include "esp_cpu.h"
#include "esp_log.h"
#include "esp_heap_caps.h"

#include "uart.h"
#include "sys/random.h"

#pragma GCC diagnostic ignored "-Wunused-function"

static void *blink_thread1(void *arg);
static void *blink_thread2(void *arg);
static void *blink_thread3(void *arg);


// pthread_mutex_t mutex;
static sem_t sema;


extern "C" void __attribute__((weak)) app_main(void)
{
    int fd = UART_createfd(0, 115200, UART_PARITY_NONE, UART_STOP_BITS_ONE);
    (void)fd;

    printf("Minimum free heap size: %d bytes\n", heap_caps_get_minimum_free_size(MALLOC_CAP_DEFAULT));
    printf("pll frequency: %llu MHz\n", clk_tree_pll_freq() / 1000000);
    printf("cpu frequency: %llu MHz\n", clk_tree_cpu_freq() / 1000000);
    printf("ahb frequency: %llu MHz\n", clk_tree_ahb_freq() / 1000000);

    if (clk_tree_module_is_enable(PERIPH_UART0_MODULE))
        printf("uart0: %lu bps\n", UART_get_baudrate(&UART0));
    if (clk_tree_module_is_enable(PERIPH_UART1_MODULE))
        printf("uart1: %lu bps\n", UART_get_baudrate(&UART1));
    if (clk_tree_module_is_enable(PERIPH_UART2_MODULE))
        printf("uart2: %lu bps\n", UART_get_baudrate(&UART2));

    for (int i = 0; i < 100; i ++)
    {
        printf("%x\n", rand());
    }

    printf("semaphore init...\n");
    sem_init(&sema, 0, 10);

    try
    {
        throw "foobar";
    }
    catch(char const *e)
    {
        printf("catched c++ exception: %s\n", e);
    }

    char const *long_text =
"1234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890\n\
1234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890\n\
1234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890\n\
1234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890\n\n\n";

    printf(long_text);
    printf(long_text);
    fflush(stdout);

    // direct io
    writebuf(fd, long_text, strlen(long_text));

    /*
    pthread_mutexattr_t attr;
    pthread_mutexattr_init(&attr);
    pthread_mutex_init(&mutex, &attr);
    */

    pthread_t id;
    pthread_create(&id, NULL, blink_thread1, NULL);
    pthread_create(&id, NULL, blink_thread2, NULL);
    pthread_create(&id, NULL, blink_thread3, NULL);

    printf("infinite loop...\n");
    while (1)
    {
        sem_post(&sema);

        printf("thread/cpu (0 <=> %d)\n", __get_CORE_ID());
        fflush(stdout);

        msleep(500);
    }
}

static void *blink_thread1(void *arg)
{
    ARG_UNUSED(arg);

    while (true)
    {
        sem_post(&sema);

        printf("thread/cpu (1 <=> %d)\n", __get_CORE_ID());
        fflush(stdout);

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
        printf("thread/cpu (2 <=> %d)\n", __get_CORE_ID());
        fflush(stdout);

        sem_timedwait_ms(&sema, 600);
    }
}

static void *blink_thread3(void *arg)
{
    ARG_UNUSED(arg);

    while (true)
    {
        printf("thread/cpu (3 <=> %d)\n", __get_CORE_ID());
        fflush(stdout);

        msleep(800);
    }
}
