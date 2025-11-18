#pragma once

class IMotor {
public:
    virtual ~IMotor() = default;

    // value from -1.0 to 1.0
    virtual void setValue(float value) = 0;

    virtual float getValue() const = 0;
};
