idf_component_register(
    SRCS "PwmMotor.cpp" "MotorGroup.cpp"
    INCLUDE_DIRS "."
    REQUIRES driver Controller
)