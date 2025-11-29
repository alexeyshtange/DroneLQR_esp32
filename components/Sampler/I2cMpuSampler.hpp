#pragma once
#include <cstdint>
#include "driver/i2c.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include "ISampler.hpp"

class I2cMpuSampler : public ISampler {
public:
    I2cMpuSampler(int sda, int scl, uint8_t addr);
    ~I2cMpuSampler();

    void captureSample() override;                            // ISR: notify task
    bool readSample(ISample& out, TickType_t timeout) override;

private:
    static void samplerTask(void* arg);

private:
    int sda;
    int scl;
    uint8_t addr;

    uint8_t* dmaRx = nullptr;        // 14 bytes + timestamp
    QueueHandle_t queue = nullptr;

    TaskHandle_t taskHandle = nullptr;
};
