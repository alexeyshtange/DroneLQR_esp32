#pragma once
#include <cstdint>

#include "driver/spi_master.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "ISampler.hpp"

class SpiMpuSampler : public ISampler {
public:
    struct Sample : ISample {
		#if USE_TIMESTAMP
        int64_t timestamp;
        #endif
        int16_t ax, ay, az;
        int16_t gx, gy, gz;
    };

    SpiMpuSampler(int miso, int mosi, int sck, int cs);
    ~SpiMpuSampler();

    void captureSample() override;                         			// ISR
    bool readSample(ISample& out, TickType_t timeout) override;  // in task, blocking mode

private:
    spi_device_handle_t dev = nullptr;
    uint8_t* dmaRx = nullptr;       // DMA buffer + optional timestamp
    spi_transaction_t trans{};
    QueueHandle_t queue = nullptr;  // queue/mailbox to pointer to dmaRx

    static void IRAM_ATTR spi_post_cb(spi_transaction_t* t);
};
