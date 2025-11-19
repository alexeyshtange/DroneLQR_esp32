#include "Drone.hpp"
#include "IController.hpp"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

Drone::Drone(int miso, int mosi, int sck, int cs, uint64_t samplePeriodUs,
             PwmMotor* m1, PwmMotor* m2, PwmMotor* m3, PwmMotor* m4)
    : mpu(miso, mosi, sck, cs),
      pid(1.0f, 0.0f, 0.0f),
      timer(TIMER_GROUP_0, TIMER_0, &mpu, samplePeriodUs),
      motors(m1, m2, m3, m4)
{
    measuredQueue = xQueueCreate(1, sizeof(Angles));
    targetQueue   = xQueueCreate(1, sizeof(Angles));
}

Drone::~Drone() {
    vQueueDelete(measuredQueue);
    vQueueDelete(targetQueue);
}

void Drone::setTargetAngles(const Angles& target) {
    xQueueOverwrite(targetQueue, &target);
}

bool Drone::getMeasuredAngles(Angles& out) {
    return xQueuePeek(measuredQueue, &out, 0) == pdTRUE;
}

void Drone::start() {
	timer.start();
    xTaskCreatePinnedToCore(taskFunc, "DroneTask", 4096, this, 15, nullptr, 1);
}

void Drone::taskFunc(void* arg) {
    Drone* self = reinterpret_cast<Drone*>(arg);
    self->loop();
}

void Drone::loop() {
    ISample sample;
    Angles measured, target;
    ControlOutput control;

    while (true) {
        // 1. get sample
        if (mpu.readSample(sample, portMAX_DELAY)) {
            filter.processSample(sample);
            measured = filter.getAngles();

            // 2. put measured
            xQueueOverwrite(measuredQueue, &measured);

            // 3. get target
            if (xQueuePeek(targetQueue, &target, 0) != pdTRUE) {
                target = {0.0f, 0.0f, 0.0f}; // default
            }

            // 4. control algorytm
            control = pid.update(measured, target, 0.010f); // dt = 10ms

            // 5. motors
            motors.applyControl(control);
            
            // 6. capture latency
            capture_latency();
        }
    }
}

void Drone::capture_latency() {
	uint64_t latency_;
    timer_get_counter_value(TIMER_GROUP_0, TIMER_0, &latency_);

    latency[index] = latency_;
    index = (index + 1) % LATENCY_HISTORY_SIZE;
}

void Drone::printf_latency() {
        uint64_t max_latency = 0;
        int idx;
        for(idx = 0; idx < LATENCY_HISTORY_SIZE; idx++) {
            if(latency[idx] > max_latency) max_latency = latency[idx];
        }
        printf("[Max latency last second]: drone task: %llu us \t", max_latency);
        mpu.printf_latency();
}