#pragma once

#include <features.h>
#include <assert.h>
#include <sys/errno.h>

#define ESP_ERROR_CHECK(x)              assert(ESP_OK == x)
#define ESP_ERROR_CHECK_WITHOUT_ABORT(x)

__BEGIN_DECLS

extern __attribute__((nothrow, const))
    char const *esp_err_to_name(esp_err_t code);

__END_DECLS
