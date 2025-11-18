idf_component_register(
    SRCS "SpiMpuSampler.cpp"
    INCLUDE_DIRS "."
    REQUIRES driver freertos esp_timer
)
