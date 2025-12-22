#pragma once

#include "Library/LiveActor/LiveActor.h"
#include "math/seadVector.h"
#include "Player/IUsePlayerCollision.h"

namespace rs {
bool calcOnGroundNormalOrGravityDir(sead::Vector3f*, const al::LiveActor*, const IUsePlayerCollision*);

} // namespace rs
