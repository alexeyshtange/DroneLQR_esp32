#pragma once
#include <cstdint>
#include "ISampler.hpp"

struct AccelGyroSample : ISample {
    float ax, ay, az;
    float gx, gy, gz;
};