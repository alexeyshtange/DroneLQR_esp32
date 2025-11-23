#include "Drone.hpp"
#include "ComplementaryFilter.hpp"
#include "PidController.hpp"
#include "SpiMpuSampler.hpp"
#include "MockSampler.hpp"
#include "PwmMotor.hpp"
#include "TelemetryUdp.hpp"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

Drone::Drone()
{
	PwmMotor::initPwm();
	motors[0] = new PwmMotor(2,  LEDC_CHANNEL_0);
	motors[1] = new PwmMotor(4, LEDC_CHANNEL_1);
	motors[2] = new PwmMotor(16, LEDC_CHANNEL_2);
	motors[3] = new PwmMotor(17, LEDC_CHANNEL_3);
	
	motorGroup = new MotorGroup();
	
	motorGroup->setMotors(motors);
	
	//sampler = new SpiMpuSampler(13, 12, 14, 15);
	sampler = new MockSampler(
    0.5f, 0.5f, 1.0f,   // accelerometer amplitudes
    0.05f, 0.05f, 0.01f, // gyroscope amplitudes
    0.02f, 0.02f, 0.02f, // accelerometer noise std
    0.005f, 0.005f, 0.002f // gyroscope noise std
	);
	
	controller = new PidController(1.0f, 0.0f, 0.0f, 0.100f);
	
	controlTimer = new ControlTimer(TIMER_GROUP_0, TIMER_0, 100000);
	controlTimer->setSampler(sampler);
	controlTimer->setMotorGroup(motorGroup);
	
	filter = new ComplementaryFilter(0.1f, 0.100f);
	
	wifiManager = new WiFiManager("ESP32-Drone", "12345678", "192.168.10.1", "192.168.10.1", "255.255.255.0");
	wifiManager->start();
	
	telemetry = new TelemetryUdp();
	telemetry->start();
}

Drone::~Drone() {

}

void Drone::start() {
	controlTimer->start();
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
    	sampler->readSample(sample, portMAX_DELAY);
    	
    	// 2. filter
        filter->processSample(sample);
        measured = filter->getAngles();

        // 3. put measured
        telemetry->putMeasuredAngles(measured);

        // 4. get target
		telemetry->getTargetAngles(target);

        // 5. control algorytm
        control = controller->update(measured, target);
        
        // 6. put control
		telemetry->putControlOutput(control);
		
        // 7. motors
        motorGroup->setControl(control);
    }
}