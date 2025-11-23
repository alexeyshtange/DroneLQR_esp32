#include "ComplementaryFilter.hpp"
#include "SpiMpuSampler.hpp"
#include "AccelGyroSample.hpp"

ComplementaryFilter::ComplementaryFilter()
    : roll(0.0f), pitch(0.0f), yaw(0.0f) {}

void ComplementaryFilter::processSample(const ISample& s) {
    const AccelGyroSample* sample = static_cast<const AccelGyroSample*>(&s);
    if (!sample) return;

    float rollAcc  = atan2f(sample->ay, sample->az) * 180.0f / M_PI;
    float pitchAcc = atan2f(-sample->ax, sqrtf(sample->ay*sample->ay + sample->az*sample->az)) * 180.0f / M_PI;

    // low-pass
    roll  = alpha * rollAcc  + (1.0f - alpha) * roll;
    pitch = alpha * pitchAcc + (1.0f - alpha) * pitch;

    // gyro integration
    yaw += sample->gx * 0.004f; // dt = 4ms
}

Angles ComplementaryFilter::getAngles() const {
    return { roll, pitch, yaw };
}
