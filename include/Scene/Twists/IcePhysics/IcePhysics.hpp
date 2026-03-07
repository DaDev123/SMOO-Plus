#pragma once

namespace al {
class Triangle;
}

class IcePhysics {
public:
    static bool sIcePhysicsEnabled;

    static bool isIcePhysicsEnabled() { return sIcePhysicsEnabled; }
    static void toggleIcePhysics() { sIcePhysicsEnabled = !sIcePhysicsEnabled; }

    static bool shouldUseIcePhysics(bool originalFloorCheck);
};

bool icePhysicsPatch(const al::Triangle& triangle, const char* floorCode);