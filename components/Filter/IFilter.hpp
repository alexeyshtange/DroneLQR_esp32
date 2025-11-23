#pragma once
#include "ISampler.hpp"
#include "Angles.hpp"

class IFilter {
public:
    virtual ~IFilter() = default;

    virtual void processSample(const ISample& s) = 0;

    virtual Angles getAngles() const = 0;
};
