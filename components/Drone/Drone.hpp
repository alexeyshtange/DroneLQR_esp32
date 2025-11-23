#pragma once
#include "SpiMpuSampler.hpp"
#include "ISampler.hpp"
#include "IFilter.hpp"
#include "IController.hpp"
#include "ControlTimer.hpp"
#include "MotorGroup.hpp"
#include "WiFiManager.hpp"
#include "ITelemetry.hpp"
#include "freertos/queue.h"

class Drone {
public:
    Drone();
    ~Drone();

    void start();
	
private:
    ISampler* sampler;
    IFilter* filter;
    IController* controller;
    IMotor* motors[4];
    MotorGroup* motorGroup;
    ControlTimer* controlTimer;
	WiFiManager* wifiManager;
	ITelemetry* telemetry;
	
    QueueHandle_t measuredQueue;
    QueueHandle_t targetQueue;

    static void taskFunc(void* arg);
    void loop();
};
