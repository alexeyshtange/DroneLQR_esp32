#include "ComplementaryFilter.hpp"
#include "AccelGyroSample.hpp"

/*struct RunningMean {
    float mean = 0.0f;
    uint32_t n = 0;

    float update(float x) {
        n++;
        mean += (x - mean) / n;
        return mean;
    }
};

static RunningMean gx_mean, gy_mean, gz_mean;

float mx = gx_mean.update(gx);
float my = gy_mean.update(gy);
float mz = gz_mean.update(gz);

printf("mean gx=%.6f gy=%.6f gz=%.6f\n", mx, my, mz);*/


ComplementaryFilter::ComplementaryFilter(float alpha, float dt)
    : alpha(alpha), dt(dt), roll(0.0f), pitch(0.0f), yaw(0.0f)
{
}

void ComplementaryFilter::processSample(const ISample& s) {
    const AccelGyroSample* sample = static_cast<const AccelGyroSample*>(&s);
    if (!sample) return;

    float rollAcc  = atan2f(sample->ay, sample->az) * 180.0f / M_PI;
    float pitchAcc = atan2f(-sample->ax, sqrtf(sample->ay*sample->ay + sample->az*sample->az)) * 180.0f / M_PI;

    // low-pass
    roll  = alpha * rollAcc  + (1.0f - alpha) * roll;
    pitch = alpha * pitchAcc + (1.0f - alpha) * pitch;

    // gyro integration
    yaw += sample->gx * dt;
    
/*    printf("%f\t%f\t%f\t%f\t%f\t%f\t%f\t%f\t%f\n",
   sample->ax, sample->ay, sample->az,
   sample->gx, sample->gy, sample->gz,
   roll, pitch, yaw);*/
}

Angles ComplementaryFilter::getAngles() const {
    return { roll, pitch, yaw };
}
