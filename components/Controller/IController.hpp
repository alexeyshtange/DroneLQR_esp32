#pragma once
#include "Angles.hpp"
#include "ControlOutput.hpp"

class IController {
public:
    virtual ~IController() = default;

    virtual ControlOutput update(const Angles& angles, const Angles& setpoints) = 0;

    virtual void reset() = 0;
};
