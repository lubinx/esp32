#ifndef __HW_PMU_H
#define __HW_PMU_H                  1

#include <features.h>

#include <stdint.h>
#include <stddef.h>

__BEGIN_DECLS

extern __attribute__((nothrow, noreturn))
    void PMU_shutdown(void);

extern __attribute__((nothrow))
    uint32_t PMU_power_acquire(void);

extern __attribute__((nothrow))
    uint32_t PMU_power_release(void);

/****************************************************************************
 *  PMU event callback
 ****************************************************************************/
    #define PMU_EVENT_ALL               (0xFF)
    #define PMU_EVENT_ON_SLEEP          (1U << 0)
    #define PMU_EVENT_ON_WAKEUP         (1U << 1)

    typedef void (* PMU_callback_t)(uint8_t, void *);

    struct PMU_notification
    {
        struct PMU_callback *next;

        PMU_callback_t callback;
        void *arg;
        uint8_t events;
    };
    typedef struct PMU_notification PMU_notification_t;

extern __attribute__((nothrow))
    void PMU_subscribe(struct PMU_notification *attr, uint8_t events, PMU_callback_t cb, void *arg);

extern __attribute__((nothrow))
    void PMU_register_notify(struct PMU_notification *ntf);

extern __attribute__((nothrow))
    void PMU_unsubscribe(struct PMU_notification *ntf);

__END_DECLS
#endif
