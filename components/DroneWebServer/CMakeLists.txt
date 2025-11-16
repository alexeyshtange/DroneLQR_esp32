idf_component_register(
    SRCS "DroneWebServer.cpp"
    INCLUDE_DIRS "."
    REQUIRES esp_http_server freertos esp_system Drone
)