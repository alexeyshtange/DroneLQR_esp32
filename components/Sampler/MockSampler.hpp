#pragma once
#include "ISampler.hpp"
#include "AccelGyroSample.hpp"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "esp_timer.h"
#include <math.h>
#include <string.h>

// very fast PRNG: xorshift64*
static inline uint64_t prng_xorshift64(uint64_t& state)
{
    state ^= state >> 12;
    state ^= state << 25;
    state ^= state >> 27;
    return state * 2685821657736338717ULL;
}

// Uniform → Normal (Box-Muller)
static inline float prng_normal(uint64_t& state)
{
    float u1 = (prng_xorshift64(state) >> 11) * (1.0f / 9007199254740992.0f);  // in (0,1)
    float u2 = (prng_xorshift64(state) >> 11) * (1.0f / 9007199254740992.0f);
    return sqrtf(-2.0f * logf(u1)) * cosf(2.0f * M_PI * u2);
}


class MockSampler : public ISampler {
public:

    MockSampler(
        float ax_amp, float ay_amp, float az_amp,
        float gx_amp, float gy_amp, float gz_amp,
        float ax_noise, float ay_noise, float az_noise,
        float gx_noise, float gy_noise, float gz_noise
    )
        : axAmp(ax_amp), ayAmp(ay_amp), azAmp(az_amp),
          gxAmp(gx_amp), gyAmp(gy_amp), gzAmp(gz_amp),
          axStd(ax_noise), ayStd(ay_noise), azStd(az_noise),
          gxStd(gx_noise), gyStd(gy_noise), gzStd(gz_noise)
    {
        queue = xQueueCreate(1, sizeof(uint64_t));
        t = 0;
        rngState = esp_timer_get_time() ^ 0xA5A55A5AA55AA5ULL;
        memset(&sampleBuf, 0, sizeof(sampleBuf));
    }

    // ISR: push only tick
    void captureSample() override
    {
        uint64_t tick = t++;

        BaseType_t hpw = pdFALSE;
        xQueueOverwriteFromISR(queue, &tick, &hpw);
        if (hpw) portYIELD_FROM_ISR();
    }

    bool readSample(ISample& out, TickType_t timeout) override
    {
        uint64_t tick;
        if (xQueueReceive(queue, &tick, timeout) != pdTRUE)
            return false;

        float rad = tick * 0.01f;

#if USE_TIMESTAMP
        sampleBuf.timestamp = esp_timer_get_time();
#endif

        // IMU-like synthetic signals
        sampleBuf.ax = axAmp * sinf(rad) + axStd * prng_normal(rngState);
        sampleBuf.ay = ayAmp * cosf(rad) + ayStd * prng_normal(rngState);
        sampleBuf.az = azAmp + azStd * prng_normal(rngState);

        sampleBuf.gx = gxAmp * sinf(rad) + gxStd * prng_normal(rngState);
        sampleBuf.gy = gyAmp * cosf(rad) + gyStd * prng_normal(rngState);
        sampleBuf.gz = gzAmp + gzStd * prng_normal(rngState);

        memcpy(&out, &sampleBuf, sizeof(AccelGyroSample));
        return true;
    }

private:
    QueueHandle_t queue;
    uint64_t t;

    uint64_t rngState;

    AccelGyroSample sampleBuf;

    float axAmp, ayAmp, azAmp;
    float gxAmp, gyAmp, gzAmp;

    float axStd, ayStd, azStd;
    float gxStd, gyStd, gzStd;
};
