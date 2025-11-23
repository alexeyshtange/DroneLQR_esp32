#pragma once
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "ISample.hpp"
#include <cstdint>

class ISampler {
public:
    virtual ~ISampler() = default;

    virtual void captureSample() = 0;

    virtual bool readSample(ISample& out, TickType_t timeout) = 0;
};