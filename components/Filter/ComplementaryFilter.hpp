#pragma once
#include "IFilter.hpp"
#include <cmath>

class ComplementaryFilter : public IFilter {
public:
    ComplementaryFilter(float alpha, float dt);

    void processSample(const ISample& s) override;

    Angles getAngles() const override;

private:
    float alpha;
    float dt;

    float roll;
    float pitch;
    float yaw;
};
