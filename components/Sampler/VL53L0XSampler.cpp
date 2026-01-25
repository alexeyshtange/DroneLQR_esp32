#include "VL53L0XSampler.hpp"
#include "DistanceSample.hpp"
#include "I2cBus.hpp"
#include "esp_timer.h"
#include <cstring>

#define REG_SYSRANGE_START      0x00
#define REG_RESULT_RANGE_STATUS 0x14
#define I2C_TIMEOUT_MS          20

VL53L0XSampler::VL53L0XSampler(uint8_t addr_) : addr(addr_) {
    queue = xQueueCreate(1, sizeof(RawRangeSample));

	wakeTof();

    BaseType_t ok = xTaskCreatePinnedToCore(
        samplerTask, "vl53l0x_sampler",
        4096, this,
        configMAX_PRIORITIES - 3,
        &taskHandle,
        1
    );

    if (ok != pdPASS) {
        printf("VL53L0XSampler: task create failed\n");
    }
}

VL53L0XSampler::~VL53L0XSampler() {
    if (taskHandle) vTaskDelete(taskHandle);
    if (queue) vQueueDelete(queue);
}

void VL53L0XSampler::wakeTof() {
    uint8_t id = 0;
    if (!I2cBus::readRegs(addr, 0xC0, &id, 1)) {
        printf("[VL53L0X] cannot read ID\n");
        return;
    }
    printf("[VL53L0X] ID=0x%02X\n", id);

    if (id != 0xEE) {
        printf("[VL53L0X] unexpected ID\n");
        return;
    }

    I2cBus::writeReg(addr, 0x88, 0x00);
    I2cBus::writeReg(addr, 0x80, 0x01);
    I2cBus::writeReg(addr, 0xFF, 0x01);
    I2cBus::writeReg(addr, 0x00, 0x00);
    I2cBus::writeReg(addr, 0x91, 0x3C);
    I2cBus::writeReg(addr, 0x00, 0x01);
    I2cBus::writeReg(addr, 0xFF, 0x00);
    I2cBus::writeReg(addr, 0x80, 0x00);

}

void IRAM_ATTR VL53L0XSampler::captureSample() {
    BaseType_t woken = pdFALSE;
    if (taskHandle) {
        vTaskNotifyGiveFromISR(taskHandle, &woken);
        if (woken) portYIELD_FROM_ISR();
    }
}

void VL53L0XSampler::samplerTask(void* arg) {
    auto* self = static_cast<VL53L0XSampler*>(arg);
    RawRangeSample sample;

    while (true) {
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

//        printf("[VL53L0X] Triggering measurement...\n");

        if (!self->startSingleMeasurement()) {
//            printf("[VL53L0X] startSingleMeasurement FAILED\n");
            vTaskDelay(pdMS_TO_TICKS(10));
            continue;
        }
//        printf("[VL53L0X] Measurement started\n");

        // Даем датчику время на измерение
        vTaskDelay(pdMS_TO_TICKS(35));

        uint16_t distance = 0;
        if (!self->readMeasurement(distance)) {
            printf("[VL53L0X] readMeasurement FAILED\n");
            vTaskDelay(pdMS_TO_TICKS(10));
            continue;
        }

        sample.distance_mm = distance;
        sample.timestamp   = esp_timer_get_time();

        printf("[VL53L0X] distance=%u mm ts=%lld\n", sample.distance_mm, sample.timestamp);

        xQueueOverwrite(self->queue, &sample);
    }
}

bool VL53L0XSampler::startSingleMeasurement() {
    bool ok = I2cBus::writeReg(addr, REG_SYSRANGE_START, 0x01);
    if (!ok) printf("[VL53L0X] I2C write to SYSRANGE_START FAILED\n");
    return ok;
}

bool VL53L0XSampler::readMeasurement(uint16_t& distance) {
    uint8_t buf[12] = {};
    bool ok = I2cBus::readRegs(addr, REG_RESULT_RANGE_STATUS, buf, sizeof(buf));
    if (!ok) {
        printf("[VL53L0X] I2C read RESULT_RANGE_STATUS FAILED\n");
        return false;
    }

    distance = (buf[10] << 8) | buf[11];
//    printf("[VL53L0X] Raw bytes: ");
//    for (int i = 0; i < 12; i++) printf("%02X ", buf[i]);
//    printf("\n");

    return true;
}


bool VL53L0XSampler::readSample(ISample& out, TickType_t timeout) {
    RawRangeSample sample;
    if (xQueueReceive(queue, &sample, timeout) != pdTRUE)
        return false;

    auto& s = static_cast<DistanceSample&>(out);
    s.distance_mm = sample.distance_mm;
    s.timestamp   = sample.timestamp;

    return true;
}
