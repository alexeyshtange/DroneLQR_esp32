#pragma once
#include "Drone.hpp"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <esp_log.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <cstring>

class DroneUdpServer {
public:
    DroneUdpServer();

    void start(Drone* dr);

private:
    Drone* drone;
    int sock;
    bool clientKnown;
    sockaddr_in clientAddr;

    static void recvTask(void* arg);
    static void sendTask(void* arg);
};
