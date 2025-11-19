#include "WiFiManager.hpp"
#include "Drone.hpp"
#include "DroneUdpServer.hpp"
#include "PwmMotor.hpp"
#include "portmacro.h"

extern "C" void app_main() {
	
	ledc_timer_config_t timer_conf{};
	timer_conf.speed_mode = LEDC_LOW_SPEED_MODE;
	timer_conf.timer_num = LEDC_TIMER_0;
	timer_conf.duty_resolution = LEDC_TIMER_8_BIT;
	timer_conf.freq_hz = 50;
	timer_conf.clk_cfg = LEDC_AUTO_CLK;
	ledc_timer_config(&timer_conf);
	
	
	static WiFiManager wifi("ESP32-Drone", "12345678", "192.168.10.1", "192.168.10.1", "255.255.255.0");

	static PwmMotor m1(2,  LEDC_CHANNEL_0, LEDC_TIMER_0);
	static PwmMotor m2(4, LEDC_CHANNEL_1, LEDC_TIMER_0);
	static PwmMotor m3(16, LEDC_CHANNEL_2, LEDC_TIMER_0);
	static PwmMotor m4(17, LEDC_CHANNEL_3, LEDC_TIMER_0);
	
	static Drone drone(13, 12, 14, 15, 10000, &m1, &m2, &m3, &m4);
	static DroneUdpServer udpServer;


    wifi.start();

    udpServer.start(&drone);

    drone.start();

    for(;;) {
        vTaskDelay(pdMS_TO_TICKS(1000));
        drone.printf_latency();
    }
}
