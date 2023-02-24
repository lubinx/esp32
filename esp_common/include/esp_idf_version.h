#pragma once

#include <sys/cdefs.h>

/** Major version number (X.x.x) */
#define ESP_IDF_VERSION_MAJOR   5
/** Minor version number (x.X.x) */
#define ESP_IDF_VERSION_MINOR   1
/** Patch version number (x.x.X) */
#define ESP_IDF_VERSION_PATCH   0

#define ESP_IDF_VERSION_VAL(major, minor, patch) ((major << 16) | (minor << 8) | (patch))
#define ESP_IDF_VERSION  ESP_IDF_VERSION_VAL(ESP_IDF_VERSION_MAJOR,  ESP_IDF_VERSION_MINOR, ESP_IDF_VERSION_PATCH)

#ifndef __ASSEMBLER__

__BEGIN_DECLS

static inline
    char const *esp_get_idf_version(void)
    {
        return IDF_VER;
    }

__END_DECLS
#endif
