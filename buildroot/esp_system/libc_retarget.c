#include <string.h>

#include "esp_attr.h"
#include "esp_newlib.h"

void esp_newlib_init(void)
{
}

// implemented in time.c
void __attribute__((weak)) esp_newlib_time_init(void)
{
}

void IRAM_ATTR esp_reent_init(struct _reent* r)
{
    memset(r, 0, sizeof(*r));
    // r->_stdout = _GLOBAL_REENT->_stdout;
    // r->_stderr = _GLOBAL_REENT->_stderr;
    // r->_stdin  = _GLOBAL_REENT->_stdin;
    // r->__cleanup = &_cleanup_r;
    // r->__sdidinit = 1;
    // r->__sglue._next = NULL;
    // r->__sglue._niobs = 0;
    // r->__sglue._iobs = NULL;
}
