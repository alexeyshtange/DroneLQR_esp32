#pragma once
#include <cstdint>
#include "ISampler.hpp"
#include "SpiBus.hpp"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"

class Pmw3901Sampler : public ISampler {
public:
    explicit Pmw3901Sampler(gpio_num_t csPin);
    ~Pmw3901Sampler();

    void captureSample() override;
    bool readSample(ISample& out, TickType_t timeout) override;

private:
    static void samplerTask(void* arg);
    void wakePmw();
    uint8_t regRead(uint8_t reg);
    void regWrite(uint8_t reg, uint8_t val);
    void initRegisters(); // сюда вставишь свои команды инициализации

private:
    gpio_num_t csPin;

    QueueHandle_t queue = nullptr;
    TaskHandle_t taskHandle = nullptr;
};
