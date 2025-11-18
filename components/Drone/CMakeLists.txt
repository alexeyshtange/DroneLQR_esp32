idf_component_register(
    SRCS "Drone.cpp"
    INCLUDE_DIRS "."
    REQUIRES freertos esp_system driver esp_timer DroneUdpServer Sampler SamplingTimer Filter Controller Motor
)
