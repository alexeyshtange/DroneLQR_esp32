#pragma once
#include "driver/gpio.h"

class LogicProbe {
public:
    static void initPin(gpio_num_t pin) {
        gpio_config_t cfg{};
        cfg.pin_bit_mask = 1ULL << pin;
        cfg.mode = GPIO_MODE_OUTPUT;
        cfg.pull_up_en = GPIO_PULLUP_DISABLE;
        cfg.pull_down_en = GPIO_PULLDOWN_DISABLE;
        cfg.intr_type = GPIO_INTR_DISABLE;
        gpio_config(&cfg);
    }

    static inline void IRAM_ATTR high(gpio_num_t pin) {
        gpio_set_level(pin, 1);
    }

    static inline void IRAM_ATTR low(gpio_num_t pin) {
        gpio_set_level(pin, 0);
    }

    static inline void IRAM_ATTR toggle(gpio_num_t pin) {
        int lvl = gpio_get_level(pin);
        gpio_set_level(pin, !lvl);
    }
};
