#include <unistd.h>
#include <stdio.h>
#include <inttypes.h>
#include <string.h>
#include <pthread.h>
#include <semaphore.h>
#include <mqueue.h>

#include <rtos/user.h>

#include "soc.h"
#include "clk-tree.h"
#include "gpio.h"
#include "uart.h"
#include "i2c.h"

#include "esp_log.h"
#include "esp_heap_caps.h"

#define LED1_PIN_NUM                    (8)
#define LED2_PIN_NUM                    (9)

#pragma GCC diagnostic ignored "-Wunused-function"

pthread_mutex_t mutex = PTHREAD_RECURSIVE_MUTEX_INITIALIZER;
int mqd;

extern "C" void I2C_test_start(void);
extern "C" void I2C_test_read(void);

static void *blink_thread(void *arg);
static void *sema_thread(void *arg);

int main(void)
{
    UART_createfd(0, 115200, UART_PARITY_NONE, UART_STOP_BITS_ONE);

    GPIO_setdir_output_pin_nb(LED1_PIN_NUM, PUSH_PULL_UP);
    GPIO_setdir_output_pin_nb(LED2_PIN_NUM, PUSH_PULL_UP);
    // IOMUX_route_output(LED1_PIN_NUM, SIG_GPIO_OUT_IDX, PUSH_PULL, false, false);
    // IOMUX_route_output(LED2_PIN_NUM, SIG_GPIO_OUT_IDX, PUSH_PULL, false, false);

    IOMUX_route_output(17, I2CEXT0_SDA_IN_IDX, OPEN_DRAIN_WITH_PULL_UP, false, false);
    IOMUX_route_output(18, I2CEXT0_SCL_IN_IDX, OPEN_DRAIN_WITH_PULL_UP, false, false);

    // I2C_configure(&I2C0, I2C_MASTER_MODE, 400);

    printf("pll frequency: %llu MHz\n", CLK_pll_freq() / 1000000);
    printf("cpu frequency: %llu MHz\n", CLK_cpu_freq() / 1000000);
    printf("ahb frequency: %llu MHz\n", CLK_ahb_freq() / 1000000);

    printf("uart0: %lu bps sclk: %llu\n", UART_get_baudrate(&UART0), CLK_uart_sclk_freq(&UART0));
    printf("uart1: %lu bps sclk: %llu\n", UART_get_baudrate(&UART1), CLK_uart_sclk_freq(&UART1));
    printf("uart2: %lu bps sclk: %llu\n", UART_get_baudrate(&UART2), CLK_uart_sclk_freq(&UART2));

    IOMUX_print();

    printf("\nMinimum free heap size: %d bytes\n", heap_caps_get_minimum_free_size(MALLOC_CAP_DEFAULT));
    printf("malloc 32k test...\n");
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

    I2C_configure(&I2C0, I2C_MASTER_MODE, 1);
    printf("i2c0: %lu bps sclk: %llu\n\n", I2C_get_bps(&I2C0), CLK_i2c_sclk_freq(&I2C0));

    I2C_test_start();
    msleep(500);
    I2C_test_read();

    mqd = mqueue_create(NULL, 4, 16);
    printf("\ninfinite loop...mqd: %d\n", mqd);
    fflush(stdout);

    pthread_t id;
    pthread_create(&id, NULL, blink_thread, NULL);
    pthread_create(&id, NULL, sema_thread, NULL);

    while (1)
    {
        I2C_test_start();
        msleep(500);
    }
}

static void *sema_thread(void *arg)
{
    ARG_UNUSED(arg);
    int step = 0;

    while (true)
    {
        mqueue_send(mqd, &step, 0);
        step ++;

        msleep(500);
    }
}

static void *blink_thread(void *arg)
{
    ARG_UNUSED(arg);
    int step = 0;

    while (true)
    {
        mqueue_recv(mqd, &step, 0);

        printf("mq recv: %d\n", step);
        fflush(stdout);

        if (step & 0x1)
        {
            GPIO_output_set_pin_nb(LED1_PIN_NUM);
            GPIO_output_clear_pin_nb(LED2_PIN_NUM);
        }
        else
        {
            GPIO_output_set_pin_nb(LED2_PIN_NUM);
            GPIO_output_clear_pin_nb(LED1_PIN_NUM);
        }
    }
}
