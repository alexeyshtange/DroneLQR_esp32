#pragma once
#include "IMotor.hpp"
#include "IController.hpp"

class MotorGroup {
public:
    static constexpr int NUM = 4;

    MotorGroup(IMotor* m1, IMotor* m2, IMotor* m3, IMotor* m4);

    void applyControl(const ControlOutput& u);

private:
    IMotor* motors[NUM];
};
