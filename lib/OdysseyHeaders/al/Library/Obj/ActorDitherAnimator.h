#pragma once

#include "Library/Anim/DitherAnimator.h"

namespace al {
class LiveActor;

// TODO: Finish this
class ActorDitherAnimator : public DitherAnimator {
public:
    ActorDitherAnimator(LiveActor* host);

    void validateDitherAnim();
    void invalidateDitherAnim();

public:
    unsigned char padding[0x68 - sizeof(DitherAnimator)];
};

static_assert(sizeof(ActorDitherAnimator) == 0x68);
}  // namespace al
