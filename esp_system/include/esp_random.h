#pragma once

#include <features.h>
#include <stdlib.h>
#include <stdint.h>
#include <sys/random.h>

__BEGIN_DECLS

static inline
    uint32_t esp_random(void)
    {
        return (uint32_t)rand();
    }

static inline
    int esp_fill_random(void *buf, size_t len)
    {
        return getrandom(buf, len, 0);
    }

__END_DECLS
