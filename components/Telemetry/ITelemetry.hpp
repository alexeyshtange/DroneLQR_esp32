#pragma once
#include "Angles.hpp"
#include "ControlOutput.hpp"

class ITelemetry {
public:
    virtual ~ITelemetry() = default;

    virtual void start() = 0;
    virtual void getTargetAngles(Angles& target) = 0;
    virtual void putMeasuredAngles(const Angles& measured) = 0;
    virtual void putControlOutput(const ControlOutput& out) = 0;
};
