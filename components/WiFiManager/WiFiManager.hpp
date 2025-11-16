#pragma once
#include <string>
#include "esp_netif.h"

class WiFiManager {
public:
    // Constructor with optional static IP, gateway and netmask
    WiFiManager(const char* ssid_, const char* password_,
                const char* ip = "192.168.4.1",
                const char* gateway = "192.168.4.1",
                const char* netmask = "255.255.255.0");

    void start();

private:
    const char* ssid;
    const char* password;
    const char* ipStr;
    const char* gwStr;
    const char* maskStr;
};
