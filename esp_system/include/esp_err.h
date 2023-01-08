#pragma once

#include <features.h>
#include <stdio.h>
#include <assert.h>

#include <sys/types.h>
#include <sys/errno.h>
#include "esp_compiler.h"

#define ESP_ERROR_CHECK(x)              assert(ESP_OK == x)
#define ESP_ERROR_CHECK_WITHOUT_ABORT(x)

__BEGIN_DECLS

extern __attribute__((nothrow, const))
    char const *esp_err_to_name(esp_err_t code);

/*
extern __attribute__((nothrow, pure))
    const char *esp_err_to_name_r(esp_err_t code, char *buf, size_t buflen);
*/

__END_DECLS
