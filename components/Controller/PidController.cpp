#include "PidController.hpp"

PidController::PidController(float kp_, float ki_, float kd_, float dt_)
    : kp(kp_), ki(ki_), kd(kd_), dt(dt_),
      integratorRoll(0), integratorPitch(0), integratorYaw(0),
      prevErrorRoll(0), prevErrorPitch(0), prevErrorYaw(0) {}

ControlOutput PidController::update(const Angles& angles, const Angles& setpoints) {
    ControlOutput out;

    float eRoll  = setpoints.roll  - angles.roll;
    float ePitch = setpoints.pitch - angles.pitch;
    float eYaw   = setpoints.yaw   - angles.yaw;

    integratorRoll  += eRoll * dt;
    integratorPitch += ePitch * dt;
    integratorYaw   += eYaw * dt;

    float dRoll  = (eRoll  - prevErrorRoll)  / dt;
    float dPitch = (ePitch - prevErrorPitch) / dt;
    float dYaw   = (eYaw   - prevErrorYaw)   / dt;

    // PID
    out.roll  = kp * eRoll  + ki * integratorRoll  + kd * dRoll;
    out.pitch = kp * ePitch + ki * integratorPitch + kd * dPitch;
    out.yaw   = kp * eYaw   + ki * integratorYaw   + kd * dYaw;

    prevErrorRoll  = eRoll;
    prevErrorPitch = ePitch;
    prevErrorYaw   = eYaw;

    return out;
}

void PidController::reset() {
    integratorRoll = integratorPitch = integratorYaw = 0;
    prevErrorRoll = prevErrorPitch = prevErrorYaw = 0;
}
