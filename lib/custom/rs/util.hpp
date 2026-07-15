#pragma once

#include "sead/math/seadVector.h"

#include "al/Library/LiveActor/LiveActor.h"

#include "game/Player/IUsePlayerCollision.h"

namespace rs {
bool calcOnGroundNormalOrGravityDir(sead::Vector3f*, const al::LiveActor*, const IUsePlayerCollision*);

}  // namespace rs
