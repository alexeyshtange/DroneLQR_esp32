idf_component_register(
    SRCS "Drone.cpp"
    INCLUDE_DIRS "."
    REQUIRES freertos esp_system DroneWebServer driver esp_timer
)
