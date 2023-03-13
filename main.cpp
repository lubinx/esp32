#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <semaphore.h>

#include <rtos/types.h>

#include "soc.h"
#include "clk_tree.h"
#include "gpio.h"
#include "uart.h"

#include "esp_log.h"
#include "esp_heap_caps.h"

#pragma GCC diagnostic ignored "-Wunused-function"

pthread_mutex_t mutex = PTHREAD_RECURSIVE_MUTEX_INITIALIZER;
static sem_t sema = SEMA_INITIALIZER(0, 100);

static void *blink_thread1(void *arg);
static void *blink_thread2(void *arg);
static void *blink_thread3(void *arg);

int main(void)
{
    int fd = UART_createfd(0, 115200, UART_PARITY_NONE, UART_STOP_BITS_ONE);
    (void)fd;

    printf("Minimum free heap size: %d bytes\n", heap_caps_get_minimum_free_size(MALLOC_CAP_DEFAULT));
    printf("pll frequency: %llu MHz\n", CLK_pll_freq() / 1000000);
    printf("cpu frequency: %llu MHz\n", CLK_cpu_freq() / 1000000);
    printf("ahb frequency: %llu MHz\n", CLK_ahb_freq() / 1000000);

    if (CLK_periph_is_enabled(PERIPH_UART0_MODULE))
        printf("uart0: %lu bps sclk: %llu\n", UART_get_baudrate(&UART0), CLK_uart_sclk_freq(&UART0));
    if (CLK_periph_is_enabled(PERIPH_UART1_MODULE))
        printf("uart1: %lu bps sclk: %llu\n", UART_get_baudrate(&UART1), CLK_uart_sclk_freq(&UART1));
    if (CLK_periph_is_enabled(PERIPH_UART2_MODULE))
        printf("uart2: %lu bps sclk: %llu\n", UART_get_baudrate(&UART2), CLK_uart_sclk_freq(&UART2));

    printf("\nmalloc 32k test...\n");
    void *ptr = malloc(32768);
    printf("\t%p\n", ptr);
    free(ptr);

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
        fflush(stdout);
    }

    char const *long_text =
"1234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890\n\
1234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890\n\
1234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890\n\
1234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890\n\n\n";

    // printf
    printf("printf: %s", long_text);
    // fprintf
    fprintf(stdout, "fprintf: %s", long_text);

    fflush(stdout);
    // direct io
    writebuf(fd, "direct i/o: ", 12);
    writebuf(fd, long_text, strlen(long_text));

    pthread_t id;
    pthread_create(&id, NULL, blink_thread1, NULL);
    pthread_create(&id, NULL, blink_thread2, NULL);
    pthread_create(&id, NULL, blink_thread3, NULL);

    printf("infinite loop...\n");
    fflush(stdout);


    while (1)
    {
        sem_post(&sema);

        RTCCNTL.time_update.update = 1;

        uint64_t t_slp = (uint64_t)RTCCNTL.slp_timer1.slp_val_hi << 32 | RTCCNTL.slp_timer0;
        uint64_t t1 = (uint64_t)RTCCNTL.time_high0.rtc_timer_value0_high << 32 | RTCCNTL.time_low0;
        uint64_t t2 = (uint64_t)RTCCNTL.time_high1.rtc_timer_value1_high << 32 | RTCCNTL.time_low1;

        printf("thread/cpu (0 <=> %d) tick: %llu/%llu/%llu\n", __get_CORE_ID(), t_slp, t1, t2);
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
