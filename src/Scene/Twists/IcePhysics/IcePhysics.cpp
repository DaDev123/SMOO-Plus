#include "Scene/Twists/IcePhysics/IcePhysics.hpp"

bool IcePhysics::sIcePhysicsEnabled = false;

bool IcePhysics::shouldUseIcePhysics(bool originalFloorCheck) {
    if (sIcePhysicsEnabled)
        return true;
    return originalFloorCheck;
}

bool icePhysicsPatch(const al::Triangle& triangle, const char* floorCode) {
    if (IcePhysics::sIcePhysicsEnabled)
        return true;
    return false;
}