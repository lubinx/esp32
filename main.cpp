#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <pthread.h>
#include <semaphore.h>

#include "spinlock.h"
#include "clk_tree.h"
#include "esp_log.h"
#include "esp_heap_caps.h"

#include "uart.h"

static void *blink_thread1(void *arg);
static void *blink_thread2(void *arg);
static void *blink_thread3(void *arg);


// pthread_mutex_t mutex;
static sem_t sema;


extern "C" void __attribute__((weak)) app_main(void)
{
    int fd = UART_createfd(0, 115200, paNone, 1);;

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

    if (0 > fd)
    {
        printf("err: %d\n", errno);
    }

    // printf("uart1: %lu bps\n", UART_get_baudrate(&UART1));
    // printf("uart2: %lu bps\n", UART_get_baudrate(&UART2));
    printf("semaphore init...\n");
    sem_init(&sema, 0, 10);

    try
    {
        throw "foobar";
    }
    catch(char const *e)
    {
        printf("catched c++ exception: %s\n", e);
        fflush(stdout);
    }
    printf("hello world!\n");
    msleep(100);

    char const *ff = "int UART_fifo_write(uart_dev_t *dev, void const *buf, unsigned count)\n\
int UART_fifo_write(uart_dev_t *dev, void const *buf, unsigned count)\n\
int UART_fifo_write(uart_dev_t *dev, void const *buf, unsigned count)\n";

    write(fd, ff, strlen(ff));
    printf(ff);
    while (0 != UART0.status.txfifo_cnt);
    printf(ff);
    while (0 != UART0.status.txfifo_cnt);

    FILE *f = fopen("foo", "w+");
    if (f)
    {
    }

    while (1)
    {
        uint8_t ch;

        read(fd, &ch, sizeof(ch));
        write(fd, &ch, sizeof(ch));
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

    printf("infinite loop...\n");
    while (1)
    {
        sem_post(&sema);

        printf("thread/cpu (0 <=> %d)\n", esp_cpu_get_core_id());
        fflush(stdout);

        msleep(500);
    }
}

static void *blink_thread1(void *arg)
{
    ARG_UNUSED(arg);

    while (true)
    {
        printf("thread/cpu (1 <=> %d)\n", esp_cpu_get_core_id());
        // fflush(stdout);

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
        printf("thread/cpu (2 <=> %d)\n", esp_cpu_get_core_id());
        // fflush(stdout);

        sem_timedwait_ms(&sema, 600);
    }
}

static void *blink_thread3(void *arg)
{
    ARG_UNUSED(arg);

    while (true)
    {
        printf("thread/cpu (3 <=> %d)\n", esp_cpu_get_core_id());
        fflush(stdout);

        msleep(800);
    }
}
