#include "SamplingTimer.hpp"
#include "esp_log.h"

SamplingTimer::SamplingTimer(timer_group_t group_, timer_idx_t timerNum_, ISampler* sampler_, uint64_t period_us)
    : group(group_), timer(timerNum_), sampler(sampler_), period(period_us)
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

SamplingTimer::~SamplingTimer() {
    stop();
}

void SamplingTimer::start() {
    timer_start(group, timer);
}

void SamplingTimer::stop() {
    timer_pause(group, timer);
}

bool IRAM_ATTR SamplingTimer::timerIsr(void* arg) {
    SamplingTimer* self = static_cast<SamplingTimer*>(arg);
    if (!self || !self->sampler) return false;

    self->sampler->captureSample();
    return true; // request scheduler
}
