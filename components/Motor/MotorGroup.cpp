#include "MotorGroup.hpp"

MotorGroup::MotorGroup() {

}

void MotorGroup::setMotors(IMotor* motors_[4]) {
	for (int i=0; i<4; ++i) motors[i] = motors_[i];
}

void MotorGroup::setControl(const ControlOutput& u) {

    float base = 0.5f;

    float m0 = base + u.pitch - u.roll + u.yaw;
    float m1 = base + u.pitch + u.roll - u.yaw;
    float m2 = base - u.pitch + u.roll + u.yaw;
    float m3 = base - u.pitch - u.roll - u.yaw;

    motors[0]->setValue(m0);
    motors[1]->setValue(m1);
    motors[2]->setValue(m2);
    motors[3]->setValue(m3);
    
    control_set_flag = true;
}

void MotorGroup::updateFromISR() {
	if(control_set_flag) {
		control_set_flag = false;
		motors[0]->updateFromISR();
	    motors[1]->updateFromISR();
	    motors[2]->updateFromISR();
	    motors[3]->updateFromISR();
    }
}