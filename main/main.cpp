#include "Drone.hpp"
#include "portmacro.h"
#include "LogicProbe.hpp"

extern "C" void app_main() {
	LogicProbe::initPin(GPIO_NUM_5);
    Drone* drone = new Drone();

    drone->start();

    for (;;) {
        vTaskDelay(portMAX_DELAY);
    }
}
