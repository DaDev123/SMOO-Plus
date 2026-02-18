#pragma once

#include "al/Library/Math/MathUtil.h"

#include <stdint.h>

#include "layouts/ShineThiefIcon.h"

class ShineThiefScore {
public:
    uint16_t mScore = 0;
    uint16_t mPrevScore = 0;

    void setTargetLayout(ShineThiefIcon* icon) { mIcon = icon; };

    void addScore(int add, const char* description) {
        mScore += add;
        mScore = al::clamp(mScore, uint16_t(0), uint16_t(9999));
        mIcon->queueScoreEvent(add, description);
    };

    void resetScore() { mScore = 0; };

    // Events
    void eventScoreDebug() { addScore(1, "Debugging!"); }
    void eventScoreSurvivalTime() { addScore(1, "Survival Time"); }
    void eventScoreRescue() { addScore(2, "Rescued"); }
    void eventScoreCapture() { addScore(2, "Captured"); }
    void eventScoreFallOff() { addScore(2, "Fell Off L"); }
    void eventScoreRunnerWin() { addScore(4, "Survived!"); }
    void eventScoreWipeout() { addScore(20, "Wipeout!"); }

private:
    ShineThiefIcon* mIcon;
};