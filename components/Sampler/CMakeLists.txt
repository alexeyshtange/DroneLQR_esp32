idf_component_register(
    SRCS "SpiMpuSampler.cpp" "I2cMpuSampler.cpp"
    INCLUDE_DIRS "."
    REQUIRES driver freertos esp_timer Sample Common
)
