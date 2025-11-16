#include "WiFiManager.hpp"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "nvs_flash.h"
#include <string.h>
#include "esp_netif_ip_addr.h"

WiFiManager::WiFiManager(const char* ssid_, const char* password_,
                         const char* ip, const char* gateway, const char* netmask)
    : ssid(ssid_), password(password_), ipStr(ip), gwStr(gateway), maskStr(netmask)
{}

void WiFiManager::start() {
    // Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if(ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND){
        nvs_flash_erase();
        nvs_flash_init();
    }

    esp_netif_init();
    esp_event_loop_create_default();

    // Create default AP interface
    esp_netif_t* ap_if = esp_netif_create_default_wifi_ap();

    // Configure static IP if provided
    if(ipStr && gwStr && maskStr){
        esp_netif_dhcps_stop(ap_if); // stop DHCP server while setting static IP
        esp_netif_ip_info_t ip_info;
        ip_info.ip.addr = esp_ip4addr_aton(ipStr);
        ip_info.gw.addr = esp_ip4addr_aton(gwStr);
        ip_info.netmask.addr = esp_ip4addr_aton(maskStr);
        esp_netif_set_ip_info(ap_if, &ip_info);
        esp_netif_dhcps_start(ap_if); // restart DHCP server
    }

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    esp_wifi_init(&cfg);

    wifi_config_t wifi_config = {};
    strncpy((char*)wifi_config.ap.ssid, ssid, sizeof(wifi_config.ap.ssid));
    wifi_config.ap.ssid_len = strlen(ssid);
    strncpy((char*)wifi_config.ap.password, password, sizeof(wifi_config.ap.password));
    wifi_config.ap.max_connection = 4;
    wifi_config.ap.authmode = WIFI_AUTH_WPA_WPA2_PSK;
    if(strlen(password) == 0) wifi_config.ap.authmode = WIFI_AUTH_OPEN;

    esp_wifi_set_mode(WIFI_MODE_AP);
    esp_wifi_set_config(WIFI_IF_AP, &wifi_config);
    esp_wifi_start();

    ESP_LOGI("WiFiManager","AP started: %s, IP: %s", ssid, ipStr);
}
