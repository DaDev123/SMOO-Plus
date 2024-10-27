#pragma once

#include <stdint.h>
#include "al/util/MathUtil.h"
#include "server/freeze-tag/FreezeTagIcon.h"

class FreezeTagScore {
    public:
        uint16_t mScore     = 0;
        uint16_t mPrevScore = 0;

        void setTargetLayout(FreezeTagIcon* icon) { mIcon = icon; };
        void initBuffer() { buffer = new char[36]; } // max length of 35 chars + \0

        void resetScore() { mScore = 0; };

        // Events
        void eventScoreDebug()        { addScore( 1, "Debugging!");    }
        void eventScoreSurvivalTime() { addScore( 1, "Survival Time"); }
        void eventScoreUnfreeze()     { addScore( 2, "Rescued");       }
        void eventScoreFreeze()       { addScore( 2, "Caught");        }
        void eventScoreFallOff()      { addScore( 2, "Fell Off L");    }
        void eventScoreRunnerWin()    { addScore( 4, "Survived!");     }
        void eventScoreWipeout()      { addScore(20, "Wipeout!");      }

        void eventNotEnoughRunnersToStart()    { addScore(0, "Not enough Runners to start a round"); }
        void eventNotEnoughChasersToStart()    { addScore(0, "Not enough Chasers to start a round"); }
        void eventNotEnoughRunnersToContinue() { addScore(0, "Round cancelled: not enough Runners"); }
        void eventNotEnoughChasersToContinue() { addScore(0, "Round cancelled: not enough Chasers"); }

        void eventRoundStarted(const char* name) {
            strcpy(buffer, "Round started by ");
            strcat(buffer, name);
            addScore(0, buffer);
        }
        void eventRoundCancelled(const char* name) {
            strcpy(buffer, "Round cancelled by ");
            strcat(buffer, name);
            addScore(0, buffer);
        }

    private:
        FreezeTagIcon* mIcon;

        char* buffer;

        void addScore(int add, const char* description) {
            mScore += add;
            mScore  = al::clamp(mScore, uint16_t(0), uint16_t(9999));
            mIcon->queueScoreEvent(add, description); // max length of 35 chars
        };
};
