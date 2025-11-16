#pragma once
#include "freertos/FreeRTOS.h"
#include "freertos/idf_additions.h"
#include "freertos/task.h"
#include "freertos/semphr.h"

class Drone {
public:
    Drone();
    void start();
    void setFrequency(float freq);
    float getValue(); // returns current sinus value

private:
    static void taskFunc(void* pvParameters);
    void loop();
    QueueHandle_t value_mailbox;
    QueueHandle_t frequency_mailbox;
    
    //Jitter
    static void jitterPrinterTask(void* pvParameters);
	static constexpr int JITTER_HISTORY_SIZE = 100;
	float jitterHistory[JITTER_HISTORY_SIZE];
	int jitterIndex = 0;
};


/*#pragma once
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include <cstdint>

#define JITTER_HISTORY_SIZE 128

class Drone {
public:
    Drone();
    void start();
    void setFrequency(float freq);
    float getValue();

private:
    static void taskFunc(void* pvParameters);
    static void jitterPrinterTask(void* pvParameters);
    void loop();

    QueueHandle_t frequency_mailbox;
    QueueHandle_t value_mailbox;

    TaskHandle_t task_handle;

    float jitterHistory[JITTER_HISTORY_SIZE]{};
    uint32_t jitterIndex{0};
};*/


