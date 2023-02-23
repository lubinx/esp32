#include "esp_err.h"

char const *esp_err_to_name(esp_err_t code)
{
    if (code >= ESP_ERR_USER_BASE)
        return "esp user error";
    if (code >= ESP_ERR_MEMPROT_BASE)
        return "esp memory protect error";
    if (code >= ESP_ERR_HW_CRYPTO_BASE)
        return "esp crypto error";
    if (code >= ESP_ERR_FLASH_BASE)
        return "esp flash error";
    if (code >= ESP_ERR_MESH_BASE)
        return "esp mesh error";
    if (code >= ESP_ERR_WIFI_BASE)
        return "esp wifi error";
    if (code >= ESP_ERR_BASE)
        return "esp error";
}
