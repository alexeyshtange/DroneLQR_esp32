#include "I2cMpuSampler.hpp"
#include "AccelGyroSample.hpp"
#include "I2cBus.hpp"
#include "esp_timer.h"
#include <cstring>
#include <cstdio>

#define I2C_TIMEOUT_MS 10

I2cMpuSampler::I2cMpuSampler(uint8_t addr_) : addr(addr_) {
    queue = xQueueCreate(1, sizeof(RawMpuSample));
    wakeMpu();

    BaseType_t ok = xTaskCreatePinnedToCore(
        samplerTask, "i2c_mpu_sampler",
        4096, this,
        configMAX_PRIORITIES - 2,
        &taskHandle,
        1
    );

    if (ok != pdPASS) {
        printf("I2cMpuSampler: task create failed\n");
    }
}

I2cMpuSampler::~I2cMpuSampler() {
    if (taskHandle) vTaskDelete(taskHandle);
    if (queue) vQueueDelete(queue);
}

void I2cMpuSampler::wakeMpu() {
    I2cBus::writeReg(addr, 0x6B, 0x00); // PWR_MGMT_1 = 0
}

void IRAM_ATTR I2cMpuSampler::captureSample() {
    BaseType_t woken = pdFALSE;
    if (taskHandle) {
        vTaskNotifyGiveFromISR(taskHandle, &woken);
        if (woken) portYIELD_FROM_ISR();
    }
}

void I2cMpuSampler::samplerTask(void* arg) {
    auto* self = static_cast<I2cMpuSampler*>(arg);
    RawMpuSample sample;

    while (true) {
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

        if (!I2cBus::readRegs(self->addr, 0x3B, sample.buf, 14)) {
            vTaskDelay(pdMS_TO_TICKS(1));
            continue;
        }

        sample.timestamp = esp_timer_get_time();
        xQueueOverwrite(self->queue, &sample);
    }
}

bool I2cMpuSampler::readSample(ISample& out, TickType_t timeout) {
    RawMpuSample sample;
    if (xQueueReceive(queue, &sample, timeout) != pdTRUE)
        return false;

    auto& s = static_cast<AccelGyroSample&>(out);

    s.timestamp = sample.timestamp;

    s.ax = (int16_t)((sample.buf[0] << 8) | sample.buf[1]) / 16384.0f;
    s.ay = (int16_t)((sample.buf[2] << 8) | sample.buf[3]) / 16384.0f;
    s.az = (int16_t)((sample.buf[4] << 8) | sample.buf[5]) / 16384.0f;

    s.gx = (int16_t)((sample.buf[8] << 8) | sample.buf[9]) / 131.0f;
    s.gy = (int16_t)((sample.buf[10] << 8) | sample.buf[11]) / 131.0f;
    s.gz = (int16_t)((sample.buf[12] << 8) | sample.buf[13]) / 131.0f;

    return true;
}
