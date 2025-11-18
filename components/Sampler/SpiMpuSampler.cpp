#include "../Sampler/SpiMpuSampler.hpp"

#include <cstring>
#include "esp_heap_caps.h"
#include "esp_timer.h"

SpiMpuSampler::SpiMpuSampler(int miso, int mosi, int sck, int cs) {
    queue = xQueueCreate(1, sizeof(uint8_t*));

    // --- SPI BUS ---
    spi_bus_config_t buscfg{};
    buscfg.mosi_io_num = mosi;
    buscfg.miso_io_num = miso;
    buscfg.sclk_io_num = sck;
    buscfg.quadwp_io_num = -1;
    buscfg.quadhd_io_num = -1;
    spi_bus_initialize(SPI2_HOST, &buscfg, SPI_DMA_CH_AUTO);

    // --- SPI DEVICE ---
    spi_device_interface_config_t devcfg{};
    devcfg.clock_speed_hz = 8 * 1000 * 1000;
    devcfg.mode = 0;
    devcfg.spics_io_num = cs;
    devcfg.queue_size = 1;
    devcfg.post_cb = spi_post_cb;
    spi_bus_add_device(SPI2_HOST, &devcfg, &dev);

    // --- DMA BUFFER ---
    dmaRx = (uint8_t*)heap_caps_malloc(32, MALLOC_CAP_DMA);
    memset(dmaRx, 0, 32);

    memset(&trans, 0, sizeof(trans));
    static uint8_t cmd = 0x3B | 0x80; // ACCEL_XOUT_H | READ
    trans.tx_buffer = &cmd;
    trans.rx_buffer = dmaRx;
    trans.length = 14 * 8;
    trans.user = this;
}

SpiMpuSampler::~SpiMpuSampler() {
    if (dev) spi_bus_remove_device(dev);
    heap_caps_free(dmaRx);
    vQueueDelete(queue);
}

void SpiMpuSampler::captureSample() {

    #if USE_TIMESTAMP
    int64_t ts = esp_timer_get_time();
    memcpy(dmaRx + 14, &ts, sizeof(ts));
	#endif

    spi_device_queue_trans(dev, &trans, 0);
}

bool SpiMpuSampler::readSample(ISample& out, TickType_t timeout) {
    uint8_t* raw;
    if (xQueueReceive(queue, &raw, timeout) != pdTRUE)
        return false;

    SpiMpuSampler::Sample& sample = static_cast<SpiMpuSampler::Sample&>(out);

    #if USE_TIMESTAMP
    memcpy(&sample.timestamp, raw + 14, sizeof(sample.timestamp));
    #endif

    sample.ax = (raw[0] << 8) | raw[1];
    sample.ay = (raw[2] << 8) | raw[3];
    sample.az = (raw[4] << 8) | raw[5];
    sample.gx = (raw[8] << 8) | raw[9];
    sample.gy = (raw[10] << 8) | raw[11];
    sample.gz = (raw[12] << 8) | raw[13];

    return true;
}

// ISR 
void IRAM_ATTR SpiMpuSampler::spi_post_cb(spi_transaction_t* t) {
    auto* self = reinterpret_cast<SpiMpuSampler*>(t->user);
    if (!self || !self->queue) return;

    #if USE_TIMESTAMP
    int64_t ts = esp_timer_get_time();
    memcpy(self->dmaRx + 14, &ts, sizeof(ts));
	#endif

    BaseType_t hpw = pdFALSE;
    xQueueOverwriteFromISR(self->queue, &self->dmaRx, &hpw);
    if (hpw) portYIELD_FROM_ISR();
}
