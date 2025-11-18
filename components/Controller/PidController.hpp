#pragma once
#include "IController.hpp"

class PidController : public IController {
public:
    PidController(float kp, float ki, float kd);

    ControlOutput update(const Angles& angles, const Angles& setpoints, float dt) override;
    void reset() override;

private:
    float kp;
    float ki;
    float kd;

    float integratorRoll;
    float integratorPitch;
    float integratorYaw;

    float prevErrorRoll;
    float prevErrorPitch;
    float prevErrorYaw;
};
