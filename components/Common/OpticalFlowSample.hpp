#pragma once
#include "ISampler.hpp"
#include <cstdint>

struct OpticalFlowSample : ISample {
    int16_t dx;
    int16_t dy;
};