#pragma once

#include <features.h>
#include <stdint.h>

__BEGIN_DECLS

extern __attribute__((nothrow, const))
    uint32_t esp_random(void);

extern __attribute__((nothrow))
    int esp_fill_random(void *buf, size_t len);

__END_DECLS
