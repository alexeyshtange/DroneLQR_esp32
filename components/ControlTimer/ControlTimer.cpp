#include "ControlTimer.hpp"
#include "MotorGroup.hpp"
#include "LogicProbe.hpp"
#include "esp_intr_alloc.h"
#include "esp_log.h"
#include "soc/gpio_num.h"

ControlTimer::ControlTimer(timer_group_t group_, timer_idx_t timerNum_, uint64_t period_us)
    : group(group_), timer(timerNum_), period(period_us)
{
    timer_config_t cfg{};
    cfg.divider = 80;               
    cfg.counter_dir = TIMER_COUNT_UP;
    cfg.counter_en = TIMER_PAUSE;
    cfg.alarm_en = TIMER_ALARM_EN;
    cfg.auto_reload = TIMER_AUTORELOAD_EN;
    cfg.intr_type = TIMER_INTR_LEVEL;

    timer_init(group, timer, &cfg);
    timer_set_counter_value(group, timer, 0);
    timer_set_alarm_value(group, timer, period);
    timer_enable_intr(group, timer);

    timer_isr_callback_add(group, timer, timerIsr, this, ESP_INTR_FLAG_IRAM);
}

ControlTimer::~ControlTimer() {
    stop();
}

void ControlTimer::setSampler(ISampler* sampler_) {
	sampler = sampler_;
}

void ControlTimer::setOrientationSampler(ISampler* sampler_) {
	orientationSampler = sampler_;
}

void ControlTimer::setDistanceSampler(ISampler* sampler_) {
	distanceSampler = sampler_;
}

void ControlTimer::setOpticalFlowSampler(ISampler* sampler_) {
	opticalFlowSampler = sampler_;
}
void ControlTimer::setMotorGroup(MotorGroup *motorGroup_) {
	motorGroup = motorGroup_;
}

void ControlTimer::start() {
    timer_start(group, timer);
}

void ControlTimer::stop() {
    timer_pause(group, timer);
}

bool IRAM_ATTR ControlTimer::timerIsr(void* arg) {	
    ControlTimer* self = static_cast<ControlTimer*>(arg);
    if (!self || !self->sampler) return false;

//    self->orientationSampler->captureSample();
    self->distanceSampler->captureSample();
//    self->opticalFlowSampler->captureSample();

    return true; // request scheduler
}

//LATENCY

void ControlTimer::capture_latency() {
	latency = timer_group_get_counter_value_in_isr(TIMER_GROUP_0, TIMER_0);
}

void ControlTimer::printf_latency() {
        printf("[ISR]: %llu us \n", latency);
}
//LATENCY