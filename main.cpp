#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

#include "esp_cpu.h"
#include "esp_heap_caps.h"
#include "esp_private/esp_clk.h"

static void *blink_thread1(void *arg);
static void *blink_thread2(void *arg);

pthread_mutex_t mutex;
int val = 0;

extern "C" void __attribute__((weak)) app_main(void)
{
    printf("Minimum free heap size: %d bytes\n", heap_caps_get_minimum_free_size(MALLOC_CAP_DEFAULT));
    printf ("cpu frequency: %d\n", esp_clk_cpu_freq());

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

    pthread_mutexattr_t attr;
    pthread_mutexattr_init(&attr);
    pthread_mutex_init(&mutex, &attr);

    pthread_t id;
    pthread_create(&id, NULL, blink_thread1, NULL);
    pthread_create(&id, NULL, blink_thread2, NULL);

    while (1)
    {
        msleep(1000);
    }
}

static void *blink_thread1(void *arg)
{
    ARG_UNUSED(arg);

    while (true)
    {
        int err = pthread_mutex_lock(&mutex);
        if (0 == err)
        {
            printf("blink thread1 cpu: %d, val %d\n", esp_cpu_get_core_id(), val);
            pthread_mutex_unlock(&mutex);
        }
        else
            printf("err %d\n", err);

        fflush(stdout);
        msleep(0);
    }
}

static void *blink_thread2(void *arg)
{
    ARG_UNUSED(arg);

    while (true)
    {
        pthread_mutex_lock(&mutex);
        val ++;

        msleep(1000);
        pthread_mutex_unlock(&mutex);
    }
}
