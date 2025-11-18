idf_component_register(
    SRCS "SamplingTimer.cpp"
    INCLUDE_DIRS "."
    REQUIRES driver freertos esp_timer Sampler
)
