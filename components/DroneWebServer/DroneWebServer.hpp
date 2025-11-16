#pragma once
#include "esp_http_server.h"
#include "Drone.hpp"

class DroneWebServer {
public:
    DroneWebServer();
    void start(Drone* drone);
    void stop();

private:
    static esp_err_t indexHandler(httpd_req_t *req);
    static esp_err_t setFreqHandler(httpd_req_t *req);
    static esp_err_t dataHandler(httpd_req_t *req);

    httpd_handle_t server;
    Drone* drone;
};
