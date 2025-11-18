#include "MpuFilter.hpp"
#include "SpiMpuSampler.hpp"

MpuFilter::MpuFilter()
    : roll(0.0f), pitch(0.0f), yaw(0.0f) {}

void MpuFilter::processSample(const ISample& s) {
    const SpiMpuSampler::Sample* sample = static_cast<const SpiMpuSampler::Sample*>(&s);
    if (!sample) return;

    // normalize acc
    float ax = static_cast<float>(sample->ax) / 16384.0f; // +/-2g
    float ay = static_cast<float>(sample->ay) / 16384.0f;
    float az = static_cast<float>(sample->az) / 16384.0f;

    float rollAcc  = atan2f(ay, az) * 180.0f / M_PI;
    float pitchAcc = atan2f(-ax, sqrtf(ay*ay + az*az)) * 180.0f / M_PI;

    // low-pass
    roll  = alpha * rollAcc  + (1.0f - alpha) * roll;
    pitch = alpha * pitchAcc + (1.0f - alpha) * pitch;

    // gyro integration
    float gx = static_cast<float>(sample->gx) / 131.0f; // +/-250 deg/s
    yaw += gx * 0.004f; // dt = 4ms
}

Angles MpuFilter::getAngles() const {
    return { roll, pitch, yaw };
}
