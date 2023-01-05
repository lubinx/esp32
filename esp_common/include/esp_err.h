#pragma once

#include <features.h>
#include <stdio.h>
#include <assert.h>

#include <sys/types.h>
#include <sys/errno.h>

#include "esp_compiler.h"

typedef int esp_err_t;

/* Definitions for error constants. */
#define ESP_OK                          0
#define ESP_FAIL                        EFAULT

#define ESP_ERR_NO_MEM                  ENOMEM
#define ESP_ERR_INVALID_ARG             EINVAL
#define ESP_ERR_NOT_SUPPORTED           EOPNOTSUPP
#define ESP_ERR_TIMEOUT                 ETIMEDOUT

#define ESP_ERR_INVALID_STATE           0x1003      /*!< Invalid state */
#define ESP_ERR_INVALID_SIZE            0x1004      /*!< Invalid size */
#define ESP_ERR_NOT_FOUND               0x1005      /*!< Requested resource not found */
#define ESP_ERR_INVALID_RESPONSE        0x1008      /*!< Received response was invalid */
#define ESP_ERR_INVALID_CRC             0x1009      /*!< CRC or checksum was invalid */
#define ESP_ERR_INVALID_VERSION         0x100A      /*!< Version was invalid */
#define ESP_ERR_INVALID_MAC             0x100B      /*!< MAC address was invalid */
#define ESP_ERR_NOT_FINISHED            0x100C      /*!< There are items remained to retrieve */


#define ESP_ERR_WIFI_BASE               0x3000      /*!< Starting number of WiFi error codes */
#define ESP_ERR_MESH_BASE               0x4000      /*!< Starting number of MESH error codes */
#define ESP_ERR_FLASH_BASE              0x6000      /*!< Starting number of flash error codes */
#define ESP_ERR_HW_CRYPTO_BASE          0xc000      /*!< Starting number of HW cryptography module error codes */
#define ESP_ERR_MEMPROT_BASE            0xd000      /*!< Starting number of Memory Protection API error codes */

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
