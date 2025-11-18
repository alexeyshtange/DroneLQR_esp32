#pragma once
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

#define USE_TIMESTAMP 1

struct ISample {
	#if USE_TIMESTAMP
    int64_t timestamp;
    #endif
};

class ISampler {
public:
    virtual ~ISampler() = default;

    virtual void captureSample() = 0;

    virtual bool readSample(ISample& out, TickType_t timeout) = 0;
};
