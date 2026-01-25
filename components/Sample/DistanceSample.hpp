#pragma once
#include <cstdint>
#include "ISampler.hpp"

struct DistanceSample : ISample {
    uint16_t distance_mm;
};
