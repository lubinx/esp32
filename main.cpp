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

// pthread_mutex_t mutex;
static sem_t sema;

static void *blink_thread1(void *arg);
static void *blink_thread2(void *arg);
static void *blink_thread3(void *arg);

extern "C" void __attribute__((weak)) app_main(void)
{
    sem_init(&sema, 0, 10);

    int fd = UART_createfd(0, 115200, UART_PARITY_NONE, UART_STOP_BITS_ONE);
    (void)fd;

    printf("Minimum free heap size: %d bytes\n", heap_caps_get_minimum_free_size(MALLOC_CAP_DEFAULT));
    printf("pll frequency: %llu MHz\n", CLK_TREE_pll_freq() / 1000000);
    printf("cpu frequency: %llu MHz\n", CLK_TREE_cpu_freq() / 1000000);
    printf("ahb frequency: %llu MHz\n", CLK_TREE_ahb_freq() / 1000000);

    if (CLK_TREE_periph_is_enable(PERIPH_UART0_MODULE))
        printf("uart0: %lu bps sclk: %llu\n", UART_get_baudrate(&UART0), CLK_TREE_uart_sclk_freq(&UART0));
    if (CLK_TREE_periph_is_enable(PERIPH_UART1_MODULE))
        printf("uart1: %lu bps sclk: %llu\n", UART_get_baudrate(&UART1), CLK_TREE_uart_sclk_freq(&UART1));
    if (CLK_TREE_periph_is_enable(PERIPH_UART2_MODULE))
        printf("uart2: %lu bps sclk: %llu\n", UART_get_baudrate(&UART2), CLK_TREE_uart_sclk_freq(&UART2));

    printf("\nrandom generator test...\n");
    for (int i = 0; i < 10; i ++)
        printf("\t0x%08x, 0x%08x, 0x%08x, 0x%08x\n", rand(), rand(), rand(), rand());

    printf("\nc++ exception test...\n");
    try
    {
        throw "foobar";
    }
    catch(char const *e)
    {
        printf("catched c++ exception: %s\n\n", e);
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
    fflush(stdout);

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
            usleep(800);
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
