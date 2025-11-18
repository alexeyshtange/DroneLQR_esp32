#pragma once
#include "ISampler.hpp"

struct Angles {
    float roll;
    float pitch;
    float yaw;
};

class IFilter {
public:
    virtual ~IFilter() = default;

    virtual void processSample(const ISample& s) = 0;

    virtual Angles getAngles() const = 0;
};
