#pragma once
#include "freertos/FreeRTOS.h"
#include "freertos/idf_additions.h"
#include "freertos/task.h"
#include <cstdint>

#define LATENCY_HISTORY_SIZE 100

class Drone {
public:
    Drone();
    void start();
    void setFrequency(float freq);
    float getValue();

private:
    static void taskFunc(void* pvParameters);
    void loop();
    static void latencyPrinterTask(void* pvParameters);

    static bool IRAM_ATTR timer_isr_cb(void* arg);

    QueueHandle_t frequency_mailbox;
    QueueHandle_t value_mailbox;
    QueueHandle_t isr_time_queue;

    int latencyHistory[LATENCY_HISTORY_SIZE] = {};
    int isrLatency[LATENCY_HISTORY_SIZE] = {};
    int latencyIndex = 0;
    int isrIndex = 0;
};
