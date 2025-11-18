#pragma once
#include "IFilter.hpp"

struct ControlOutput {
    float roll;
    float pitch;
    float yaw;
};

class IController {
public:
    virtual ~IController() = default;

    virtual ControlOutput update(const Angles& angles, const Angles& setpoints, float dt) = 0;

    virtual void reset() = 0;
};
