idf_component_register(
    SRCS "ControlTimer.cpp"
    INCLUDE_DIRS "."
    REQUIRES driver freertos esp_timer Sampler Motor Common
)
