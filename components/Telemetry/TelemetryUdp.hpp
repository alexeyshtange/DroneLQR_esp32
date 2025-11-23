#pragma once
#include "ITelemetry.hpp"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_log.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <cstring>
#include <cstdio>

class TelemetryUdp : public ITelemetry {
public:
    TelemetryUdp();
    ~TelemetryUdp() override;

    void start() override;
    void stop();

    void putMeasuredAngles(const Angles& measured) override;
    void getTargetAngles(Angles& target) override;
    void putControlOutput(const ControlOutput& control) override;

    void disconnectFromClient();

private:
    static void recvTask(void* arg);
    static void sendTask(void* arg);
    void checkClientTimeout();

private:
    static constexpr const char* TAG = "TelemetryUdp";

    int sock = -1;

    sockaddr_in clientAddr{};
    bool clientKnown = false;
    TickType_t lastClientTick = 0;

    QueueHandle_t measuredQueue;
    QueueHandle_t targetQueue;
    QueueHandle_t controlOutputQueue;

    TaskHandle_t recvTaskHandle = nullptr;
    TaskHandle_t sendTaskHandle = nullptr;

    bool running = true;
};
