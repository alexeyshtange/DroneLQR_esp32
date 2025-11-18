#pragma once
#include "IFilter.hpp"
#include <cmath>

class MpuFilter : public IFilter {
public:
    MpuFilter();

    void processSample(const ISample& s) override;

    Angles getAngles() const override;

private:
    float roll;
    float pitch;
    float yaw;

    float alpha = 0.1f; // low-pass coefficient
};
