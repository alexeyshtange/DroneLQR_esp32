#pragma once
#include <cstdint>
#include "driver/i2c.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

class I2cBus {
public:
    static void init(gpio_num_t sda, gpio_num_t scl, uint32_t freq_hz);

    static bool writeReg(uint8_t addr, uint8_t reg, uint8_t val);
    static bool writeRegs(uint8_t addr, uint8_t reg, const uint8_t* data, size_t len);
    static bool readRegs(uint8_t addr, uint8_t reg, uint8_t* data, size_t len);

private:
    static SemaphoreHandle_t mutex;
};
