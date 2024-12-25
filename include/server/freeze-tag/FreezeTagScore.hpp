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
        void eventScoreSurvivalTime() { addScore( 1, "Temps de Survie"); }
        void eventScoreUnfreeze()     { addScore( 2, "Sauvé");       }
        void eventScoreFreeze()       { addScore( 2, "Attrapé");        }
        void eventScoreFallOff()      { addScore( 2, "Tomber L + Ratio");    }
        void eventScoreRunnerWin()    { addScore( 4, "Survécu!");     }
        void eventScoreWipeout()      { addScore(20, "Wipeout!");      }

        void eventNotEnoughRunnersToStart()    { addScore(0, "Pas assez de Joueurs pour commencer la Partie"); }
        void eventNotEnoughChasersToStart()    { addScore(0, "Pas assez de Loups pour commencer la Partie"); }
        void eventNotEnoughRunnersToContinue() { addScore(0, "Partie annulé: Pas assez de Joueurs"); }
        void eventNotEnoughChasersToContinue() { addScore(0, "Partie annulé: Pas assez de Loups"); }

        void eventRoundStarted(const char* name) {
            strcpy(buffer, "Partie commencé part ");
            strcat(buffer, name);
            addScore(0, buffer);
        }
        void eventRoundCancelled(const char* name) {
            strcpy(buffer, "Partie arreté par ");
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
