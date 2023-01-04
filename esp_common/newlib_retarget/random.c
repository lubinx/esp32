#include <features.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/random.h>

#include "esp_random.h"

void srand(unsigned seed)
{
    ARG_UNUSED(seed);
}

int rand(void)
{
    int buf;
    esp_fill_random(&buf, sizeof(buf));

    return buf;
}


ssize_t getrandom(void *buf, size_t buflen, unsigned int flags)
{
    if (buf == NULL) {
        errno = EFAULT;
        return -1;
    }

    esp_fill_random(buf, buflen);
    return buflen;
}
