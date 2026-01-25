#include "SpiBus.hpp"
#include <cstring>
#include <cstdio>

SemaphoreHandle_t SpiBus::mutex = nullptr;
spi_host_device_t SpiBus::hostPort = SPI_HOST_MAX; // неинициализирован
SpiBus::DeviceEntry SpiBus::devices[MAX_DEVICES] = {};

void SpiBus::init(spi_host_device_t host, gpio_num_t mosi, gpio_num_t miso, gpio_num_t sclk) {
    if (mutex) return; // уже инициализировано
    mutex = xSemaphoreCreateMutex();
    hostPort = host;

    spi_bus_config_t buscfg{};
    buscfg.mosi_io_num = mosi;
    buscfg.miso_io_num = miso;
    buscfg.sclk_io_num = sclk;
    buscfg.quadwp_io_num = -1;
    buscfg.quadhd_io_num = -1;
    buscfg.max_transfer_sz = 32; // под motion регистры достаточно
    spi_bus_initialize(host, &buscfg, 0);
}

bool SpiBus::addDevice(gpio_num_t csPin, uint32_t clkHz) {
    for (int i = 0; i < MAX_DEVICES; ++i) {
        if (devices[i].handle == nullptr) {
            spi_device_interface_config_t devcfg{};
            devcfg.clock_speed_hz = clkHz; // 1 MHz
            devcfg.mode = 3;
            devcfg.spics_io_num = -1; // ручной CS
            devcfg.queue_size = 1;

            if (spi_bus_add_device(hostPort, &devcfg, &devices[i].handle) != ESP_OK)
                return false;

            devices[i].csPin = csPin;
            return true;
        }
    }
    return false;
}

spi_device_handle_t SpiBus::findDevice(gpio_num_t csPin) {
    for (int i = 0; i < MAX_DEVICES; ++i) {
        if (devices[i].handle && devices[i].csPin == csPin)
            return devices[i].handle;
    }
    return nullptr;
}

bool SpiBus::writeReg(gpio_num_t csPin, uint8_t reg, uint8_t val) {
    if (mutex) xSemaphoreTake(mutex, portMAX_DELAY);

	spi_device_handle_t device = findDevice(csPin);

    gpio_set_level(csPin, 0);

    uint8_t buf[2] = { static_cast<uint8_t>(reg | 0x80u), val };
    spi_transaction_t t{};
    t.length = 16;
    t.tx_buffer = buf;
    esp_err_t rc = spi_device_transmit(device, &t);

    gpio_set_level(csPin, 1);

    if (mutex) xSemaphoreGive(mutex);
    return rc == ESP_OK;
}

bool SpiBus::writeRegs(gpio_num_t csPin, uint8_t reg, const uint8_t* data, size_t len) {
    if (mutex) xSemaphoreTake(mutex, portMAX_DELAY);

	spi_device_handle_t device = findDevice(csPin);

    gpio_set_level(csPin, 0);

    spi_transaction_t t{};
    uint8_t header = reg | 0x80u;
    t.length = 8 * (len + 1);
    uint8_t* txBuf = new uint8_t[len + 1];
    txBuf[0] = header;
    memcpy(txBuf + 1, data, len);
    t.tx_buffer = txBuf;
    esp_err_t rc = spi_device_transmit(device, &t);
    delete[] txBuf;

    gpio_set_level(csPin, 1);

    if (mutex) xSemaphoreGive(mutex);
    return rc == ESP_OK;
}

bool SpiBus::readRegs(gpio_num_t csPin, uint8_t reg, uint8_t* data, size_t len) {
    if (mutex) xSemaphoreTake(mutex, portMAX_DELAY);

	spi_device_handle_t device = findDevice(csPin);

    gpio_set_level(csPin, 0);

    uint8_t addr = reg & ~0x80u;
    spi_transaction_t t{};
    uint8_t* txBuf = new uint8_t[len + 1];
    uint8_t* rxBuf = new uint8_t[len + 1];
    txBuf[0] = addr;
    memset(txBuf + 1, 0, len);
    t.length = 8 * (len + 1);
    t.tx_buffer = txBuf;
    t.rx_buffer = rxBuf;
    esp_err_t rc = spi_device_transmit(device, &t);

    if (rc == ESP_OK) memcpy(data, rxBuf + 1, len);

    delete[] txBuf;
    delete[] rxBuf;

    gpio_set_level(csPin, 1);

    if (mutex) xSemaphoreGive(mutex);
    return rc == ESP_OK;
}
