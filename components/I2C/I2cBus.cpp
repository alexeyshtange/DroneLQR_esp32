#include "I2cBus.hpp"
#include <cstring>

SemaphoreHandle_t I2cBus::mutex = nullptr;

void I2cBus::init(gpio_num_t sda, gpio_num_t scl, uint32_t freq_hz) {
    static bool initialized = false;
    if (initialized) return;

    mutex = xSemaphoreCreateMutex();

    i2c_config_t cfg{};
    cfg.mode = I2C_MODE_MASTER;
    cfg.sda_io_num = sda;
    cfg.scl_io_num = scl;
    cfg.master.clk_speed = freq_hz;

    i2c_param_config(I2C_NUM_0, &cfg);
    i2c_driver_install(I2C_NUM_0, cfg.mode, 0, 0, 0);

    initialized = true;
}

bool I2cBus::writeReg(uint8_t addr, uint8_t reg, uint8_t val) {
    if (mutex) xSemaphoreTake(mutex, portMAX_DELAY);

    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (addr << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write(cmd, &reg, 1, true);
    i2c_master_write(cmd, &val, 1, true);
    i2c_master_stop(cmd);

    esp_err_t rc = i2c_master_cmd_begin(I2C_NUM_0, cmd, pdMS_TO_TICKS(20));
    i2c_cmd_link_delete(cmd);

    if (mutex) xSemaphoreGive(mutex);
    return rc == ESP_OK;
}

bool I2cBus::writeRegs(uint8_t addr, uint8_t reg, const uint8_t* data, size_t len) {
    if (mutex) xSemaphoreTake(mutex, portMAX_DELAY);

    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (addr << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write(cmd, &reg, 1, true);
    if (len > 0) i2c_master_write(cmd, data, len, true);
    i2c_master_stop(cmd);

    esp_err_t rc = i2c_master_cmd_begin(I2C_NUM_0, cmd, pdMS_TO_TICKS(20));
    i2c_cmd_link_delete(cmd);

    if (mutex) xSemaphoreGive(mutex);
    return rc == ESP_OK;
}

bool I2cBus::readRegs(uint8_t addr, uint8_t reg, uint8_t* data, size_t len) {
    if (mutex) xSemaphoreTake(mutex, portMAX_DELAY);

    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (addr << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write(cmd, &reg, 1, true);
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (addr << 1) | I2C_MASTER_READ, true);
    if (len > 1) {
        i2c_master_read(cmd, data, len - 1, I2C_MASTER_ACK);
    }
    i2c_master_read_byte(cmd, data + len - 1, I2C_MASTER_NACK);
    i2c_master_stop(cmd);

    esp_err_t rc = i2c_master_cmd_begin(I2C_NUM_0, cmd, pdMS_TO_TICKS(20));
    i2c_cmd_link_delete(cmd);

    if (mutex) xSemaphoreGive(mutex);
    return rc == ESP_OK;
}
