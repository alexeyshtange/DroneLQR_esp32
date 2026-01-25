idf_component_register(
    SRCS "SpiMpuSampler.cpp" "I2cMpuSampler.cpp" "VL53L0XSampler.cpp" "Pmw3901Sampler.cpp"
    INCLUDE_DIRS "."
    REQUIRES driver freertos esp_timer Sample Common I2C SPI
)
