#include "Drone.hpp"
#include "esp_timer.h"
#include "driver/ledc.h"
#include "freertos/idf_additions.h"
#include <cmath>
#include <stdio.h>

Drone::Drone() {
    frequency_mailbox = xQueueCreate(1, sizeof(float));
    value_mailbox = xQueueCreate(1, sizeof(float));
    float init = 1.0f;
    xQueueOverwrite(frequency_mailbox, &init);
    float vinit = 0.0f;
    xQueueOverwrite(value_mailbox, &vinit);

    // configure LEDC PWM on pin 2
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
}

void Drone::start() {
	xTaskCreatePinnedToCore(taskFunc, "DroneTask", 4096, this, 15, NULL, 1); // core 1
    xTaskCreatePinnedToCore(jitterPrinterTask, "JitterPrinter", 2048, this, 5, NULL, 1);
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
    const TickType_t period = pdMS_TO_TICKS(10); // 100 Hz
    TickType_t lastWakeTime = xTaskGetTickCount();
    
    float frequency, value;

    while(true) {
        vTaskDelayUntil(&lastWakeTime, period);
        TickType_t ticks = xTaskGetTickCount();
        
        xQueuePeek(frequency_mailbox, &frequency, 0);
        float t = (float)ticks / configTICK_RATE_HZ;

        // sine and PWM
        value = sinf(2.0f * M_PI * frequency * t);
        xQueueOverwrite(value_mailbox, &value);
        uint32_t duty = (uint32_t)((value + 1.0f) / 2.0f * pwmMax);
        ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, duty);
        ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0);

		// Jitter
		const int64_t period_us = 10000; // 10 ms
		static int64_t last_t_us = 0;
		
		int64_t t_us = esp_timer_get_time();
		float jitter_percent = 0.0f;
		if(last_t_us != 0){
		    float jitter_us = static_cast<float>(t_us - last_t_us - period_us);
		    jitter_percent = (jitter_us / period_us) * 100.0f;
		}

		jitterHistory[jitterIndex] = jitter_percent;
		jitterIndex = (jitterIndex + 1) % JITTER_HISTORY_SIZE;
		
		last_t_us = t_us;
	    }
}

void Drone::jitterPrinterTask(void* pvParameters) {
    Drone* self = (Drone*)pvParameters;
    while(true) {
        vTaskDelay(pdMS_TO_TICKS(1000)); // раз в 1 сек
        float maxJitter = 0.0f;
        for(int i=0; i<JITTER_HISTORY_SIZE; i++){
            float absJ = fabsf(self->jitterHistory[i]);
            if(absJ > maxJitter) maxJitter = absJ;
        }
        printf("Max jitter last second: %.2f%%\n", maxJitter);
    }
}



/*#include "Drone.hpp"
#include "esp_timer.h"
#include "driver/ledc.h"
#include "freertos/idf_additions.h"
#include <cmath>
#include <stdio.h>

static esp_timer_handle_t hard_timer;
#define TIMER_PERIOD_US 10000  // 100 Hz

void IRAM_ATTR drone_timer_isr(void* arg)
{
    TaskHandle_t task_handle = static_cast<TaskHandle_t>(arg);
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    xTaskNotifyFromISR(task_handle, 1, eSetValueWithOverwrite, &xHigherPriorityTaskWoken);
    if (xHigherPriorityTaskWoken) portYIELD_FROM_ISR();
}

Drone::Drone() {
    frequency_mailbox = xQueueCreate(1, sizeof(float));
    value_mailbox = xQueueCreate(1, sizeof(float));

    float init = 1.0f;
    float vinit = 0.0f;
    xQueueOverwrite(frequency_mailbox, &init);
    xQueueOverwrite(value_mailbox, &vinit);

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
    ledc_channel_config(&ch_conf);
}

void Drone::start() {
    xTaskCreatePinnedToCore(taskFunc, "DroneTask", 4096, this, 15, &task_handle, 1);
    xTaskCreatePinnedToCore(jitterPrinterTask, "JitterPrinter", 2048, this, 5, NULL, 1);

    const esp_timer_create_args_t args = {
        .callback = &drone_timer_isr,
        .arg = this->task_handle,
        .name = "hard100Hz"
    };
    esp_timer_create(&args, &hard_timer);
    esp_timer_start_periodic(hard_timer, TIMER_PERIOD_US);
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
    int64_t last_t_us = 0;

    float frequency, value;

    while (true) {
        uint32_t note;
        xTaskNotifyWait(0, 0, &note, portMAX_DELAY);

        int64_t t_us = esp_timer_get_time();

        if (last_t_us != 0) {
            float jitter = (float)(t_us - last_t_us - TIMER_PERIOD_US);
            float jitter_percent = (jitter / TIMER_PERIOD_US) * 100.0f;
            jitterHistory[jitterIndex] = jitter_percent;
            jitterIndex = (jitterIndex + 1) % JITTER_HISTORY_SIZE;
        }
        last_t_us = t_us;

        xQueuePeek(frequency_mailbox, &frequency, 0);
        float t = (float)t_us / 1e6f;

        value = sinf(2.0f * M_PI * frequency * t);
        xQueueOverwrite(value_mailbox, &value);

        uint32_t duty = (uint32_t)((value + 1.0f) * 0.5f * pwmMax);
        ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, duty);
        ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0);
    }
}

void Drone::jitterPrinterTask(void* pvParameters) {
    Drone* self = (Drone*)pvParameters;
    while (true) {
        vTaskDelay(pdMS_TO_TICKS(1000));
        float maxJitter = 0.0f;
        for (int i = 0; i < JITTER_HISTORY_SIZE; i++) {
            float j = fabsf(self->jitterHistory[i]);
            if (j > maxJitter) maxJitter = j;
        }
        printf("Max jitter last second: %.2f%%\n", maxJitter);
    }
}
*/