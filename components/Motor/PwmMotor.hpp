#pragma once
#include "IMotor.hpp"
#include "driver/ledc.h"

class PwmMotor : public IMotor {
public:
    PwmMotor(int gpio, ledc_channel_t channel, ledc_timer_t timer = LEDC_TIMER_0);
    void setValue(float value) override;
    float getValue() const override;

private:
    int gpioNum;
    ledc_channel_t channel;
    ledc_timer_t timerNum;
    float currentValue;
    static const int pwmMax = 255;
};
