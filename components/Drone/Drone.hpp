#pragma once
#include "SpiMpuSampler.hpp"
#include "MpuFilter.hpp"
#include "IController.hpp"
#include "PidController.hpp"
#include "SamplingTimer.hpp"
#include "MotorGroup.hpp"
#include "PwmMotor.hpp"
#include "freertos/queue.h"

#define LATENCY_HISTORY_SIZE 256

class Drone {
public:
    Drone(int miso, int mosi, int sck, int cs, uint64_t samplePeriodUs,
          PwmMotor* m1, PwmMotor* m2, PwmMotor* m3, PwmMotor* m4);
    ~Drone();

    void setTargetAngles(const Angles& target);
    bool getMeasuredAngles(Angles& out);

    void start();
    
    void printf_latency();
private:
    SpiMpuSampler mpu;
    MpuFilter filter;
    PidController pid;
    SamplingTimer timer;

    MotorGroup motors;

    QueueHandle_t measuredQueue;
    QueueHandle_t targetQueue;

    static void taskFunc(void* arg);
    void loop();
    
    volatile uint64_t latency[LATENCY_HISTORY_SIZE];
	volatile int index = 0;
	void capture_latency();
};
