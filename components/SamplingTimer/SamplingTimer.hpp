#pragma once
#include "driver/timer.h"
#include "driver/timer_types_legacy.h"
#include <cstdint>
#include "../Sampler/ISampler.hpp"

class SamplingTimer {
public:
    SamplingTimer(timer_group_t group, timer_idx_t timerNum, ISampler* sampler, uint64_t period_us);
    ~SamplingTimer();

    void start();
    void stop();

private:
    timer_group_t group;
    timer_idx_t timer;
    ISampler* sampler;
    uint64_t period;

    static bool IRAM_ATTR timerIsr(void* arg);
};
