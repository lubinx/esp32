#ifndef __ESP32S3_PHY_H
#define __ESP32S3_PHY_H                 1

#include <features.h>

typedef int esp_err_t;

enum esp_mac_type_t
{
    ESP_MAC_WIFI_STA,
    ESP_MAC_WIFI_SOFTAP,
    ESP_MAC_BT,
    ESP_MAC_ETH,
    ESP_MAC_IEEE802154,
    ESP_MAC_BASE,
    ESP_MAC_EFUSE_FACTORY,
    ESP_MAC_EFUSE_CUSTOM,
};
typedef enum esp_mac_type_t         esp_mac_type_t;

__BEGIN_DECLS


    esp_err_t esp_efuse_mac_get_default(uint8_t *mac);
    esp_err_t esp_read_mac(uint8_t *mac, esp_mac_type_t type);

__END_DECLS
#endif
