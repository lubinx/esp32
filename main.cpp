#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <semaphore.h>
#include <mqueue.h>

#include "soc.h"
#include "clk-tree.h"
#include "gpio.h"
#include "uart.h"

#include "esp_log.h"
#include "esp_heap_caps.h"

#include "efuse.h"
#include <soc/efuse_struct.h>

#define LED1_PIN_NUM                    (8)
#define LED2_PIN_NUM                    (9)

int mqd;

static void *blink_thread(void *arg);
static void *sema_thread(void *arg);

int main(void)
{
    UART_createfd(0, 115200, UART_PARITY_NONE, UART_STOP_BITS_ONE);

    GPIO_setdir_output_pin_nb(LED1_PIN_NUM, PUSH_PULL_UP);
    GPIO_setdir_output_pin_nb(LED2_PIN_NUM, PUSH_PULL_UP);

    IOMUX_route_output(17, I2CEXT0_SDA_IN_IDX, OPEN_DRAIN_WITH_PULL_UP, false, false);
    IOMUX_route_output(18, I2CEXT0_SCL_IN_IDX, OPEN_DRAIN_WITH_PULL_UP, false, false);

    uint8_t MAC[6];
    EFUSE_read_blob(EFUSE_FIELD_MAC, &MAC, 48);
    printf("MAC: %02X:%02X:%02X:%02X:%02X:%02X\n", MAC[0], MAC[1], MAC[2], MAC[3], MAC[4], MAC[5]);

    printf("pll frequency: %lu MHz\n", CLK_pll_freq() / 1000000);
    printf("cpu frequency: %lu MHz\n", CLK_cpu_freq() / 1000000);
    printf("ahb frequency: %lu MHz\n", CLK_ahb_freq() / 1000000);

    printf("uart0: %lu bps sclk: %lu\n", UART_get_baudrate(&UART0), CLK_uart_sclk_freq(&UART0));
    printf("uart1: %lu bps sclk: %lu\n", UART_get_baudrate(&UART1), CLK_uart_sclk_freq(&UART1));
    printf("uart2: %lu bps sclk: %lu\n", UART_get_baudrate(&UART2), CLK_uart_sclk_freq(&UART2));

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

    struct mq_attr mq_attr = MQATTR_INITIALIZER(16, 4);

    mqd = mq_open("mq", O_CREAT | O_RDWR, 0644, &mq_attr);
    printf("\ninfinite loop...mqd: %d\n", mqd);
    fflush(stdout);

    pthread_t id;
    pthread_create(&id, NULL, blink_thread, NULL);
    pthread_create(&id, NULL, sema_thread, NULL);

    while (1)
    {
        printf("main thread: %u\n", __get_CORE_ID());
        fflush(stdout);

        msleep(1000);
    }
}

static void *sema_thread(void *arg)
{
    ARG_UNUSED(arg);
    int step = 0;

    while (true)
    {
        mq_send(mqd, &step, sizeof(step), 0);
        step ++;

        printf("sema thread: %u\n", __get_CORE_ID());
        fflush(stdout);

        msleep(500);
    }
}

static void *blink_thread(void *arg)
{
    ARG_UNUSED(arg);
    int step = 0;

    while (true)
    {
        mq_receive(mqd, &step, sizeof(step), 0);

        printf("blink thread: %u\n", __get_CORE_ID());
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
