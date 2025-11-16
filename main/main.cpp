#include "WiFiManager.hpp"
#include "Drone.hpp"
#include "DroneWebServer.hpp"
#include "portmacro.h"

extern "C" {
	void app_main(void);
}

void app_main() {
    static WiFiManager wifi("ESP32-Drone", "12345678", "192.168.10.1", "192.168.10.1", "255.255.255.0");
    wifi.start();

  	static Drone drone;
    static DroneWebServer webServer;

    webServer.start(&drone);

    // Start drone task (PWM sinus)
    drone.start();

    for(;;){
        vTaskDelay(1000/portTICK_PERIOD_MS);
    }
}
