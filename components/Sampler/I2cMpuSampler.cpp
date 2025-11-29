#include "I2cMpuSampler.hpp"
#include "AccelGyroSample.hpp"
#include "esp_timer.h"
#include "driver/timer.h"
#include "driver/timer_types_legacy.h"
#include "esp_heap_caps.h"
#include <cstring>
#include <cstdio>

#define I2C_PORT  I2C_NUM_0
#define I2C_TIMEOUT_MS 10

I2cMpuSampler::I2cMpuSampler(int sda_, int scl_, uint8_t addr_)
    : sda(sda_), scl(scl_), addr(addr_)
{
    queue = xQueueCreate(1, sizeof(uint8_t*));
    dmaRx = (uint8_t*)heap_caps_malloc(32, MALLOC_CAP_8BIT);
    if (!dmaRx) {
        printf("I2cMpuSampler: malloc failed\n");
        return;
    }
    memset(dmaRx, 0, 32);

    // Configure I2C master
    i2c_config_t cfg{};
    cfg.mode = I2C_MODE_MASTER;
    cfg.sda_io_num = sda;
    cfg.scl_io_num = scl;
    cfg.master.clk_speed = 400000;    // 400kHz default; change to 1MHz only if hardware supports
    i2c_param_config(I2C_PORT, &cfg);
    esp_err_t r = i2c_driver_install(I2C_PORT, cfg.mode, 0, 0, 0);
    if (r != ESP_OK) {
        printf("I2cMpuSampler: i2c_driver_install failed: %d\n", r);
    }

    // Wake device: write 0x00 to PWR_MGMT_1 (0x6B)
    {
        i2c_cmd_handle_t cmd = i2c_cmd_link_create();
        uint8_t reg = 0x6B;
        uint8_t val = 0x00;
        i2c_master_start(cmd);
        i2c_master_write_byte(cmd, (addr << 1) | I2C_MASTER_WRITE, true);
        i2c_master_write(cmd, &reg, 1, true);
        i2c_master_write(cmd, &val, 1, true);
        i2c_master_stop(cmd);
        esp_err_t rc = i2c_master_cmd_begin(I2C_PORT, cmd, pdMS_TO_TICKS(I2C_TIMEOUT_MS));
        i2c_cmd_link_delete(cmd);
        if (rc != ESP_OK) {
            printf("I2cMpuSampler: wake PWR_MGMT_1 failed: %d\n", rc);
        }
    }

    // Optional: set sample rate divider, config, ranges if needed
    // (omitted for brevity — add writes to SMPLRT_DIV, CONFIG, GYRO_CONFIG, ACCEL_CONFIG)

    // Create sampler task (high priority)
    BaseType_t ok = xTaskCreatePinnedToCore(
        samplerTask, "i2c_sampler",
        4096, this,
        configMAX_PRIORITIES - 2,
        &taskHandle, // task handle stored for notifications
        1
    );
    if (ok != pdPASS) {
        printf("I2cMpuSampler: task create failed\n");
    }
}

I2cMpuSampler::~I2cMpuSampler() {
    if (taskHandle) {
        vTaskDelete(taskHandle);
        taskHandle = nullptr;
    }
    i2c_driver_delete(I2C_PORT);
    if (dmaRx) heap_caps_free(dmaRx);
    if (queue) vQueueDelete(queue);
}

void IRAM_ATTR I2cMpuSampler::captureSample() {
    // ISR-safe: notify the sampler task
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    if (taskHandle) {
        vTaskNotifyGiveFromISR(taskHandle, &xHigherPriorityTaskWoken);
        if (xHigherPriorityTaskWoken) portYIELD_FROM_ISR();
    }
}

void I2cMpuSampler::samplerTask(void* arg) {
    auto* self = static_cast<I2cMpuSampler*>(arg);
    // Ensure task handle is set (it is set by xTaskCreate)
    // Initial read to flush registers not necessary; burst read will fetch current values.

    while (true) {
        // Wait for notification from ISR (clears on take)
        // Block indefinitely — wakes only when notified
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
        // --- Read MPU6050 burst 14 bytes starting at 0x3B ---
        uint8_t reg = 0x3B;  // ACCEL_XOUT_H

        i2c_cmd_handle_t cmd = i2c_cmd_link_create();
        i2c_master_start(cmd);
        i2c_master_write_byte(cmd, (self->addr << 1) | I2C_MASTER_WRITE, true);
        i2c_master_write(cmd, &reg, 1, true);

        i2c_master_start(cmd);
        i2c_master_write_byte(cmd, (self->addr << 1) | I2C_MASTER_READ, true);
        i2c_master_read(cmd, self->dmaRx, 14, I2C_MASTER_LAST_NACK);
        i2c_master_stop(cmd);

        esp_err_t rc = i2c_master_cmd_begin(I2C_PORT, cmd, pdMS_TO_TICKS(I2C_TIMEOUT_MS));
        i2c_cmd_link_delete(cmd);

        if (rc != ESP_OK) {
            // read failed; optionally retry once (simple retry)
            printf("I2cMpuSampler: read failed %d\n", rc);
            // small backoff to avoid busy retry storms
            vTaskDelay(pdMS_TO_TICKS(1));
            continue;
        }

        // Append timestamp (microseconds)
        int64_t ts = esp_timer_get_time();
        memcpy(self->dmaRx + 14, &ts, sizeof(ts));

        // mailbox
        printf("[I2C]: %lld us", timer_group_get_counter_value_in_isr(TIMER_GROUP_0, TIMER_0));
        xQueueOverwrite(self->queue, &self->dmaRx);
    }
}

bool I2cMpuSampler::readSample(ISample& out, TickType_t timeout) {
    uint8_t* raw = nullptr;
    if (xQueueReceive(queue, &raw, timeout) != pdTRUE)
        return false;

    auto& s = static_cast<AccelGyroSample&>(out);

    memcpy(&s.timestamp, raw + 14, sizeof(s.timestamp));

	// parse as signed 16-bit (two's complement)
	int16_t raw_ax = (int16_t)((raw[0] << 8) | raw[1]);
	int16_t raw_ay = (int16_t)((raw[2] << 8) | raw[3]);
	int16_t raw_az = (int16_t)((raw[4] << 8) | raw[5]);
	
	int16_t raw_gx = (int16_t)((raw[8]  << 8) | raw[9]);
	int16_t raw_gy = (int16_t)((raw[10] << 8) | raw[11]);
	int16_t raw_gz = (int16_t)((raw[12] << 8) | raw[13]);
	
	s.ax = raw_ax / 16384.0f;
	s.ay = raw_ay / 16384.0f;
	s.az = raw_az / 16384.0f;
	
	s.gx = raw_gx / 131.0f;
	s.gy = raw_gy / 131.0f;
	s.gz = raw_gz / 131.0f;
/*	printf("raw: %02X %02X %02X %02X %02X %02X  %02X %02X  %02X %02X %02X %02X %02X %02X | "
	       "ax=%.3f ay=%.3f az=%.3f gx=%.3f gy=%.3f gz=%.3f ts=%lld\n",
	       raw[0],raw[1],raw[2],raw[3],raw[4],raw[5],
	       raw[6],raw[7],
	       raw[8],raw[9],raw[10],raw[11],raw[12],raw[13],
	       s.ax,s.ay,s.az,s.gx,s.gy,s.gz,s.timestamp);*/

    return true;
}
