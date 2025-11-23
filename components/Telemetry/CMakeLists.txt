idf_component_register(
    SRCS "TelemetryUdp.cpp"
    INCLUDE_DIRS "."
    REQUIRES esp_http_server freertos esp_system Common
)