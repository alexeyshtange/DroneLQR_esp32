#include "Pmw3901Sampler.hpp"
#include "OpticalFlowSample.hpp"
#include "SpiBus.hpp"
#include "esp_timer.h"
#include <cstring>
#include <cstdio>

Pmw3901Sampler::Pmw3901Sampler(gpio_num_t cs) : csPin(cs) {
    queue = xQueueCreate(1, sizeof(OpticalFlowSample));
    gpio_set_direction((gpio_num_t)csPin, GPIO_MODE_OUTPUT);
    gpio_set_level((gpio_num_t)csPin, 1);

	SpiBus::addDevice(csPin, 1000000);

    wakePmw();
    initRegisters();

    BaseType_t ok = xTaskCreatePinnedToCore(
        samplerTask, "spi_pmw_sampler",
        4096, this,
        configMAX_PRIORITIES - 2,
        &taskHandle,
        1
    );

    if (ok != pdPASS) {
        printf("Pmw3901Sampler: task create failed\n");
    }
}

Pmw3901Sampler::~Pmw3901Sampler() {
    if (taskHandle) vTaskDelete(taskHandle);
    if (queue) vQueueDelete(queue);
}

void Pmw3901Sampler::wakePmw() {
    // Power-on reset
    regWrite(0x3A, 0x5A);
    vTaskDelay(pdMS_TO_TICKS(5));

    // Чтение ID для проверки устройства
    uint8_t chipId      = regRead(0x00);
    uint8_t chipIdInv   = regRead(0x5F);

    printf("Pmw3901Sampler: CS=%u, CHIP_ID=0x%02X, CHIP_ID_INVERSE=0x%02X\n",
           csPin, chipId, chipIdInv);
}


void IRAM_ATTR Pmw3901Sampler::captureSample() {
    BaseType_t woken = pdFALSE;
    if (taskHandle) {
        vTaskNotifyGiveFromISR(taskHandle, &woken);
        if (woken) portYIELD_FROM_ISR();
    }
}

void Pmw3901Sampler::samplerTask(void* arg) {
    auto* self = static_cast<Pmw3901Sampler*>(arg);
    OpticalFlowSample sample;

    while (true) {
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

        uint8_t motion = self->regRead(0x02); // обязательно сначала читать Motion
        sample.dx = (int16_t)((self->regRead(0x04) << 8) | self->regRead(0x03));
        sample.dy = (int16_t)((self->regRead(0x06) << 8) | self->regRead(0x05));
        sample.timestamp = esp_timer_get_time();

//        printf("[FLOW] ts=%llu dx=%d dy=%d motion=0x%02X xL=0x%02X xH=0x%02X yL=0x%02X yH=0x%02X\n",
//               sample.timestamp,
//               sample.dx,
//               sample.dy,
//               motion,
//               self->regRead(0x03),
//               self->regRead(0x04),
//               self->regRead(0x05),
//               self->regRead(0x06));

        xQueueOverwrite(self->queue, &sample);
    }
}



bool Pmw3901Sampler::readSample(ISample& out, TickType_t timeout) {
    OpticalFlowSample sample;
    if (xQueueReceive(queue, &sample, timeout) != pdTRUE)
        return false;

    auto& s = static_cast<OpticalFlowSample&>(out);
    s.dx = sample.dx;
    s.dy = sample.dy;
    s.timestamp = sample.timestamp;

    return true;
}

// Низкоуровневые SPI доступ через SpiBus
uint8_t Pmw3901Sampler::regRead(uint8_t reg) {
    uint8_t val = 0;
    SpiBus::readRegs(csPin, reg & ~0x80u, &val, 1);
    return val;
}

void Pmw3901Sampler::regWrite(uint8_t reg, uint8_t val) {
    SpiBus::writeReg(csPin, reg | 0x80u, val);
}

void Pmw3901Sampler::initRegisters() {
  regWrite(0x7F, 0x00);
  regWrite(0x61, 0xAD);
  regWrite(0x7F, 0x03);
  regWrite(0x40, 0x00);
  regWrite(0x7F, 0x05);
  regWrite(0x41, 0xB3);
  regWrite(0x43, 0xF1);
  regWrite(0x45, 0x14);
  regWrite(0x5B, 0x32);
  regWrite(0x5F, 0x34);
  regWrite(0x7B, 0x08);
  regWrite(0x7F, 0x06);
  regWrite(0x44, 0x1B);
  regWrite(0x40, 0xBF);
  regWrite(0x4E, 0x3F);
  regWrite(0x7F, 0x08);
  regWrite(0x65, 0x20);
  regWrite(0x6A, 0x18);
  regWrite(0x7F, 0x09);
  regWrite(0x4F, 0xAF);
  regWrite(0x5F, 0x40);
  regWrite(0x48, 0x80);
  regWrite(0x49, 0x80);
  regWrite(0x57, 0x77);
  regWrite(0x60, 0x78);
  regWrite(0x61, 0x78);
  regWrite(0x62, 0x08);
  regWrite(0x63, 0x50);
  regWrite(0x7F, 0x0A);
  regWrite(0x45, 0x60);
  regWrite(0x7F, 0x00);
  regWrite(0x4D, 0x11);
  regWrite(0x55, 0x80);
  regWrite(0x74, 0x1F);
  regWrite(0x75, 0x1F);
  regWrite(0x4A, 0x78);
  regWrite(0x4B, 0x78);
  regWrite(0x44, 0x08);
  regWrite(0x45, 0x50);
  regWrite(0x64, 0xFF);
  regWrite(0x65, 0x1F);
  regWrite(0x7F, 0x14);
  regWrite(0x65, 0x60);
  regWrite(0x66, 0x08);
  regWrite(0x63, 0x78);
  regWrite(0x7F, 0x15);
  regWrite(0x48, 0x58);
  regWrite(0x7F, 0x07);
  regWrite(0x41, 0x0D);
  regWrite(0x43, 0x14);
  regWrite(0x4B, 0x0E);
  regWrite(0x45, 0x0F);
  regWrite(0x44, 0x42);
  regWrite(0x4C, 0x80);
  regWrite(0x7F, 0x10);
  regWrite(0x5B, 0x02);
  regWrite(0x7F, 0x07);
  regWrite(0x40, 0x41);
  regWrite(0x70, 0x00);

  vTaskDelay(pdMS_TO_TICKS(100));
  
  regWrite(0x32, 0x44);
  regWrite(0x7F, 0x07);
  regWrite(0x40, 0x40);
  regWrite(0x7F, 0x06);
  regWrite(0x62, 0xf0);
  regWrite(0x63, 0x00);
  regWrite(0x7F, 0x0D);
  regWrite(0x48, 0xC0);
  regWrite(0x6F, 0xd5);
  regWrite(0x7F, 0x00);
  regWrite(0x5B, 0xa0);
  regWrite(0x4E, 0xA8);
  regWrite(0x5A, 0x50);
  regWrite(0x40, 0x80);
}
