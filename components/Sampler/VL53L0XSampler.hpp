#pragma once
#include <cstdint>
#include "ISampler.hpp"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"

class VL53L0XSampler : public ISampler {
public:
    VL53L0XSampler(uint8_t addr);
    ~VL53L0XSampler();

    void captureSample() override;                   
    bool readSample(ISample& out, TickType_t timeout) override;

private:
    static void samplerTask(void* arg);
    void wakeTof();
    bool startSingleMeasurement();
    bool readMeasurement(uint16_t& distance);

private:
    uint8_t addr;
    struct RawRangeSample {
        uint16_t distance_mm;
        int64_t timestamp;
    };

    QueueHandle_t queue = nullptr;
    TaskHandle_t taskHandle = nullptr;
};
