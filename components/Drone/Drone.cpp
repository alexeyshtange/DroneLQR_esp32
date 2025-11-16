#include "Drone.hpp"
#include "esp_timer.h"
#include "driver/ledc.h"
#include "driver/timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include <cmath>

Drone::Drone() {
    frequency_mailbox = xQueueCreate(1, sizeof(float));
    value_mailbox = xQueueCreate(1, sizeof(float));
    isr_time_queue = xQueueCreate(1, sizeof(int64_t));

    float init = 1.0f;
    xQueueOverwrite(frequency_mailbox, &init);
    float vinit = 0.0f;
    xQueueOverwrite(value_mailbox, &vinit);

    // PWM
    ledc_timer_config_t timer_conf = {};
    timer_conf.speed_mode = LEDC_LOW_SPEED_MODE;
    timer_conf.timer_num = LEDC_TIMER_0;
    timer_conf.duty_resolution = LEDC_TIMER_8_BIT;
    timer_conf.freq_hz = 50;
    timer_conf.clk_cfg = LEDC_AUTO_CLK;
    ledc_timer_config(&timer_conf);

    ledc_channel_config_t ch_conf = {};
    ch_conf.gpio_num = 2;
    ch_conf.speed_mode = LEDC_LOW_SPEED_MODE;
    ch_conf.channel = LEDC_CHANNEL_0;
    ch_conf.intr_type = LEDC_INTR_DISABLE;
    ch_conf.timer_sel = LEDC_TIMER_0;
    ch_conf.duty = 0;
    ch_conf.hpoint = 0;
    ch_conf.flags.output_invert = 0;
    ledc_channel_config(&ch_conf);

    // HW timer using callback
    timer_config_t tcfg = {};
    tcfg.divider = 80;               // 1 tick = 1 us
    tcfg.counter_dir = TIMER_COUNT_UP;
    tcfg.counter_en = TIMER_PAUSE;
    tcfg.alarm_en = TIMER_ALARM_EN;
    tcfg.auto_reload = TIMER_AUTORELOAD_EN;
    tcfg.intr_type = TIMER_INTR_LEVEL;
    timer_init(TIMER_GROUP_0, TIMER_0, &tcfg);
    timer_set_counter_value(TIMER_GROUP_0, TIMER_0, 0);
    timer_set_alarm_value(TIMER_GROUP_0, TIMER_0, 10000); // 10ms
    timer_enable_intr(TIMER_GROUP_0, TIMER_0);

    timer_isr_callback_add(TIMER_GROUP_0, TIMER_0, timer_isr_cb, this, ESP_INTR_FLAG_IRAM);
    timer_start(TIMER_GROUP_0, TIMER_0);
}

bool IRAM_ATTR Drone::timer_isr_cb(void* arg) {
    Drone* self = (Drone*)arg;
    int64_t now = esp_timer_get_time();

    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    xQueueOverwriteFromISR(self->isr_time_queue, &now, &xHigherPriorityTaskWoken);
    
    uint64_t latency;
	timer_get_counter_value(TIMER_GROUP_0, TIMER_0, &latency);
    self->isrLatency[self->isrIndex] = latency;
    self->isrIndex = (self->isrIndex + 1) % LATENCY_HISTORY_SIZE;
    
    return true; // request scheduler
}

void Drone::start() {
    xTaskCreatePinnedToCore(taskFunc, "DroneTask", 4096, this, 15, NULL, 1);
    xTaskCreatePinnedToCore(latencyPrinterTask, "latencyLogger", 2048, this, 5, NULL, 1);
}

void Drone::setFrequency(float freq) {
    xQueueOverwrite(frequency_mailbox, &freq);
}

float Drone::getValue() {
    float val = 0.0f;
    xQueuePeek(value_mailbox, &val, 0);
    return val;
}

void Drone::taskFunc(void* pvParameters) {
    Drone* self = (Drone*)pvParameters;
    self->loop();
}

void Drone::loop() {
    const int pwmMax = 255;
    float frequency, value;
    int64_t last_time = 0;

    while(true) {
        int64_t timestamp;
        if(xQueueReceive(isr_time_queue, &timestamp, portMAX_DELAY)) {
            if(timestamp == last_time) continue;
            last_time = timestamp;

            xQueuePeek(frequency_mailbox, &frequency, 0);
            float t = (float)timestamp / 1000000.0f;
            value = sinf(2.0f * M_PI * frequency * t);
            xQueueOverwrite(value_mailbox, &value);

            uint32_t duty = (uint32_t)((value + 1.0f) / 2.0f * pwmMax);
            ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, duty);
            ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0);
            
            uint64_t latency;
			timer_get_counter_value(TIMER_GROUP_0, TIMER_0, &latency);
            latencyHistory[latencyIndex] = latency;
            latencyIndex = (latencyIndex + 1) % LATENCY_HISTORY_SIZE;
        }
    }
}

void Drone::latencyPrinterTask(void* pvParameters) {
    Drone* self = (Drone*)pvParameters;
    while(true) {
        vTaskDelay(pdMS_TO_TICKS(1000));
        int64_t maxISR = 0;
        int64_t maxTask = 0;
        for(int i = 0; i < LATENCY_HISTORY_SIZE; i++){
            if(self->isrLatency[i] > maxISR) maxISR = self->isrLatency[i];
            if(self->latencyHistory[i] > maxTask) maxTask = self->latencyHistory[i];
        }
        printf("Max ISR latency: %lld us | Max Task latency: %lld us\n", maxISR, maxTask);
    }
}
