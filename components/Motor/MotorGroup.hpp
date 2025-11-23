#pragma once
#include "IMotor.hpp"
#include "ControlOutput.hpp"

class MotorGroup {
public:
    static constexpr int NUM = 4;

    MotorGroup();
    
    void setMotors(IMotor* motors_[NUM]);

    void setControl(const ControlOutput& u);
    
    void updateFromISR();

private:
    IMotor* motors[NUM];
    bool control_set_flag = 0;
};
