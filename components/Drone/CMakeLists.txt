idf_component_register(
    SRCS "Drone.cpp"
    INCLUDE_DIRS "."
    REQUIRES freertos esp_system driver esp_timer Sampler ControlTimer Filter Controller Motor WiFiManager Telemetry
)
