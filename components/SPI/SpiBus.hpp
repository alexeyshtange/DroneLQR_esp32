#pragma once
#include <cstdint>
#include "driver/spi_master.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

#define MAX_DEVICES 4

class SpiBus {
public:
    static void init(spi_host_device_t host, gpio_num_t mosi, gpio_num_t miso, gpio_num_t sclk);
    static bool addDevice(gpio_num_t csPin, uint32_t clkHz);
    static spi_device_handle_t findDevice(gpio_num_t csPin);
    
    static bool writeReg(gpio_num_t csPin, uint8_t reg, uint8_t val);
    static bool writeRegs(gpio_num_t csPin, uint8_t reg, const uint8_t* data, size_t len);
    static bool readRegs(gpio_num_t csPin, uint8_t reg, uint8_t* data, size_t len);

private:
    static SemaphoreHandle_t mutex;
    static spi_host_device_t hostPort;
    
    struct DeviceEntry {
    gpio_num_t csPin;
    spi_device_handle_t handle;
	};

	static DeviceEntry devices[MAX_DEVICES];
};
