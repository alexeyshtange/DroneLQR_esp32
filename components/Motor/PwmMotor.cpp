#include "PwmMotor.hpp"

PwmMotor::PwmMotor(int gpio, ledc_channel_t channel_, ledc_timer_t timer_)
    : gpioNum(gpio), channel(channel_), timerNum(timer_), currentValue(0.0f)
{
    // timer should be already configured
    ledc_channel_config_t ch_conf{};
    ch_conf.gpio_num = gpioNum;
    ch_conf.speed_mode = LEDC_LOW_SPEED_MODE;
    ch_conf.channel = channel;
    ch_conf.intr_type = LEDC_INTR_DISABLE;
    ch_conf.timer_sel = timerNum;
    ch_conf.duty = 0;
    ch_conf.hpoint = 0;
    ch_conf.flags.output_invert = 0;
    ledc_channel_config(&ch_conf);
}

void PwmMotor::initPwm() {
	ledc_timer_config_t timer_conf{};
	timer_conf.speed_mode = LEDC_LOW_SPEED_MODE;
	timer_conf.timer_num = LEDC_TIMER_0;
	timer_conf.duty_resolution = LEDC_TIMER_8_BIT;
	timer_conf.freq_hz = 50;
	timer_conf.clk_cfg = LEDC_AUTO_CLK;
	ledc_timer_config(&timer_conf);
}

void PwmMotor::setValue(float value) {
	value = value/45;
    if (value > 1.0f) value = 1.0f;
    if (value < -1.0f) value = -1.0f;
    currentValue = value;

    // convert -1..1 to 0..255
    duty = (uint32_t)((value + 1.0f) / 2.0f * pwmMax);
}

void PwmMotor::updateFromISR() {
    ledc_set_duty(LEDC_LOW_SPEED_MODE, channel, duty);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, channel);
}

float PwmMotor::getValue() const {
    return currentValue;
}
