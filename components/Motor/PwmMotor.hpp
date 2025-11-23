#pragma once
#include "IMotor.hpp"
#include "driver/ledc.h"
#include <cstdint>

class PwmMotor : public IMotor {
public:
    PwmMotor(int gpio, ledc_channel_t channel, ledc_timer_t timer = LEDC_TIMER_0);
    static void initPwm();
    void setValue(float value) override;
    void updateFromISR() override;
    float getValue() const override;

private:
    int gpioNum;
    ledc_channel_t channel;
    ledc_timer_t timerNum;
    float currentValue;
    uint32_t duty;
    static const int pwmMax = 255;
};
