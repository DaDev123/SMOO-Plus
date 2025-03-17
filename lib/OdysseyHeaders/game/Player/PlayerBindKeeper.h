#pragma once

namespace al {
class HitSensor;
}

class PlayerBindKeeper {
public:
    al::HitSensor* getBindSensor() const { return mBindSensor; }

public:
    char padding[8];
    al::HitSensor* mBindSensor;
};
