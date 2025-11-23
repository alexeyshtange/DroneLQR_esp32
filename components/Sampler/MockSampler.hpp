#pragma once
#include "ISampler.hpp"
#include "AccelGyroSample.hpp"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "esp_timer.h"
#include <cstring>
#include <math.h>

class MockSampler : public ISampler {
public:
    MockSampler() {
        queue = xQueueCreate(1, sizeof(uint64_t));
        t = 0;
        memset(&sampleBuf, 0, sizeof(sampleBuf));
    }

    // ISR: only pushes counter into queue, no computations
    void captureSample() override {
        uint64_t tick = t++;
        BaseType_t hpw = pdFALSE;
        xQueueOverwriteFromISR(queue, &tick, &hpw);
        if (hpw) portYIELD_FROM_ISR();
    }

    bool readSample(ISample& out, TickType_t timeout) override {
        uint64_t tick;
        if (xQueueReceive(queue, &tick, timeout) != pdTRUE)
            return false;

        float rad = tick * 0.01f;

#if USE_TIMESTAMP
        sampleBuf.timestamp = esp_timer_get_time();
#endif

        // synth data + noise
        sampleBuf.ax = 0.5f * sinf(rad) + noise();
        sampleBuf.ay = 0.5f * cosf(rad) + noise();
        sampleBuf.az = 1.0f + noise();

        sampleBuf.gx = 0.05f * sinf(rad) + noise();
        sampleBuf.gy = 0.05f * cosf(rad) + noise();
        sampleBuf.gz = noise();

        memcpy(&out, &sampleBuf, sizeof(AccelGyroSample));
        return true;
    }

private:
    QueueHandle_t queue;
    uint64_t t;
    AccelGyroSample sampleBuf;

    // pseudo-noise based on timer
    float noise() {
        uint64_t now = esp_timer_get_time();
        float n = (now & 0x3FF) / 1024.0f;
        return (n - 0.5f) * 0.02f;
    }
};
