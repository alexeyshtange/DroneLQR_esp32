#include "MotorGroup.hpp"

MotorGroup::MotorGroup(IMotor* m1, IMotor* m2, IMotor* m3, IMotor* m4) {
    motors[0] = m1;
    motors[1] = m2;
    motors[2] = m3;
    motors[3] = m4;
}

void MotorGroup::applyControl(const ControlOutput& u) {

    float base = 0.5f;

    float m0 = base + u.pitch - u.roll + u.yaw;
    float m1 = base + u.pitch + u.roll - u.yaw;
    float m2 = base - u.pitch + u.roll + u.yaw;
    float m3 = base - u.pitch - u.roll - u.yaw;

    motors[0]->setValue(m0);
    motors[1]->setValue(m1);
    motors[2]->setValue(m2);
    motors[3]->setValue(m3);
}
