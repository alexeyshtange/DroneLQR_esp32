#include "Drone.hpp"
#include "TelemetryUdp.hpp"
#include "freertos/idf_additions.h"
#include "freertos/projdefs.h"
#include "portmacro.h"
#include "LogicProbe.hpp"
#include "I2cBus.hpp"
#include "SpiBus.hpp"
#include "I2cMpuSampler.hpp"
#include "VL53L0XSampler.hpp"
#include "Pmw3901Sampler.hpp"
#include "DistanceSample.hpp"
#include "AccelGyroSample.hpp"
#include "OpticalFlowSample.hpp"

extern "C" void app_main() {
    I2cBus::init(
	GPIO_NUM_21, 
    GPIO_NUM_22, 
	400000);

    SpiBus::init(
        VSPI_HOST,
        GPIO_NUM_19,
        GPIO_NUM_23,
        GPIO_NUM_18
    );

    printf("Scanning I2C bus:\n");
    for (uint8_t i = 1; i < 127; i++) {
        uint8_t dummy;
        if (I2cBus::readRegs(i, 0xC0, &dummy, 1)) {
            printf("Found device at 0x%02X, ID=0x%02X\n", i, dummy);
        }
    }

    static I2cMpuSampler   mpuSampler(0x68);
    static VL53L0XSampler  tofSampler(0x29);
    static Pmw3901Sampler  flowSampler(GPIO_NUM_5);

    static ControlTimer* controlTimer =
        new ControlTimer(TIMER_GROUP_0, TIMER_0, 100000);

    controlTimer->setOrientationSampler(&mpuSampler);
    controlTimer->setDistanceSampler(&tofSampler);
    controlTimer->setOpticalFlowSampler(&flowSampler);
    controlTimer->start();

    AccelGyroSample   mpuData;
    DistanceSample    tofData;
    OpticalFlowSample flowData;
    
    static TelemetryUdp telemetry;
    static WiFiManager wifiManager("ESP32-Drone", "12345678", "192.168.10.1", "192.168.10.1", "255.255.255.0");
	wifiManager.start();
	telemetry.start();
    
 	Angles measured;
 	
    for (;;) {
//        if (mpuSampler.readSample(mpuData, pdMS_TO_TICKS(100))) {
//            printf("[MPU] ts=%lld ax=%.3f ay=%.3f az=%.3f gx=%.3f gy=%.3f gz=%.3f\n",
//                   mpuData.timestamp,
//                   mpuData.ax, mpuData.ay, mpuData.az,
//                   mpuData.gx, mpuData.gy, mpuData.gz);
//        }
//
        if (tofSampler.readSample(tofData, pdMS_TO_TICKS(100))) {
            printf("[TOF] ts=%lld distance=%u mm\n",
                   tofData.timestamp,
                   tofData.distance_mm);
	        measured.roll = tofData.distance_mm;
			telemetry.putMeasuredAngles(measured);
        }
//
//        if (flowSampler.readSample(flowData, pdMS_TO_TICKS(500))) {
////		printf("[FLOW] ts=%llu dx=%d dy=%d\n",
////		       flowData.timestamp,
////		       flowData.dx,
////		       flowData.dy);
//
//		measured.pitch = flowData.dx;
//		measured.roll = flowData.dy;
//		telemetry.putMeasuredAngles(measured);
//        }

//        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
