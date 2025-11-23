#pragma once
#include "MotorGroup.hpp"
#include "ISampler.hpp"
#include "driver/timer.h"
#include "driver/timer_types_legacy.h"
#include <cstdint>

class ControlTimer {
public:
    ControlTimer(timer_group_t group, timer_idx_t timerNum, uint64_t period_us);
    ~ControlTimer();

	void setSampler(ISampler* sampler);
	void setMotorGroup(MotorGroup *motorGroup);
    void start();
    void stop();

private:
    timer_group_t group;
    timer_idx_t timer;
    ISampler* sampler;
    MotorGroup* motorGroup;
    uint64_t period;

    static bool IRAM_ATTR timerIsr(void* arg);
};
