#pragma once

#include "al/camera/CameraTicket.h"
#include "game/Player/PlayerActorBase.h"
#include "game/Player/PlayerActorHakoniwa.h"
#include "puppets/PuppetInfo.h"
#include "sead/math/seadVector.h"
#include "server/freeze-tag/FreezeTagInfo.h"
#include "server/freeze-tag/FreezeTagPackets.hpp"
#include "server/gamemode/GameModeBase.hpp"
#include "server/gamemode/GameModeTimer.hpp"

class FreezeHintArrow;
class FreezePlayerBlock;
class FreezeTagIcon;

struct FreezePostProcessingTypes {
    enum Type : u8 { // Snapshot mode post processing state
        PPDISABLED    = 0,
        PPFROZEN      = 1,
        PPENDGAMELOSE = 2,
        PPENDGAMEWIN  = 3,
    };
};
typedef FreezePostProcessingTypes::Type FreezePostProcessingType;

class FreezeTagMode : public GameModeBase {
    public:
        // implemented in FreezeTagMode.cpp:
        FreezeTagMode(const char* name);
        void init(GameModeInitInfo const& info) override;
        void processPacket(Packet* packet) override;
        Packet* createPacket() override;
        void begin() override;
        void end() override;
        void pause() override;
        void unpause() override;
        void update() override;
        bool showNameTag(PuppetInfo* other) override;
        bool showNameTagEverywhere(PuppetActor* other) override;
        void debugMenuControls(sead::TextWriter* gTextWriter) override;
        void debugMenuPlayer(sead::TextWriter* gTextWriter, PuppetInfo* other = nullptr) override;
        void sendFreezePacket(FreezeUpdateType updateType);                    // Called instead of Client::sendGameModeInfPacket(), allows setting packet type
        void onHakoniwaSequenceFirstStep(HakoniwaSequence* sequence) override; // Called with HakoniwaSequence hook, wipe used in recovery event

        // implemented here:
        inline bool     ignoreComboBtn()       const override { return true;  }
        inline bool     pauseTimeWhenPaused()  const override { return true;  }
        inline bool     isUseNormalUI()        const override { return false; }
        inline bool     hasCustomCamera()      const override { return true;  }
        inline bool     isScoreEventsEnabled() const { return mIsScoreEventsValid;       }
        inline bool     isHost()               const { return mInfo->isHost();           }
        inline bool     isRound()              const { return mInfo->isRound();          }
        inline bool     isPlayerRunner()       const { return mInfo->isPlayerRunner();   }
        inline bool     isPlayerChaser()       const { return mInfo->isPlayerChaser();   }
        inline bool     isPlayerFrozen()       const { return mInfo->isPlayerFrozen();   }
        inline bool     isPlayerUnfrozen()     const { return mInfo->isPlayerUnfrozen(); }
        inline bool     isWipeout()            const { return mIsEndgameActive;          }
        inline uint16_t getScore()             const { return mInfo->getScore();         }
        inline int      runners()              const { return mInfo->runners();          }
        inline int      chasers()              const { return mInfo->chasers();          }
        inline int      others()               const { return mInfo->others();           }

        // implemented in FreezeTagModeTrigger.cpp:
        void startRound(int roundMinutes);                              // Actives round on this specific client
        void endRound(bool isAbort);                                    // Ends round, allows setting for if this was a natural end or abort (used for scoring)
        bool trySetPlayerRunnerState(FreezeState state);                // Sets runner to alive or frozen, many safety checks
        void tryStartEndgameEvent();                                    // Starts the WIPEOUT message event
        bool tryStartRecoveryEvent(bool isEndgame);                     // Returns player to a chaser's position or last stood position, unless endgame variant
        bool tryEndRecoveryEvent();                                     // Called after the fade of the recovery event
        void tryScoreEvent(FreezeTagPacket* packet, PuppetInfo* other); // Attempt to score when getting a packet that a runner near us was frozen
        bool trySetPostProcessingType(FreezePostProcessingType type);   // Sets the post processing type, also used for disabling
        void warpToRecoveryPoint(al::LiveActor* actor);                 // Warps runner to chaser OR if impossible, last standing position

        // implemented in FreezeTagModeCam.cpp:
        void createCustomCameraTicket(al::CameraDirector* director) override;
        void updateSpectateCam(PlayerActorBase* playerBase); // Updates the frozen spectator camera

        // implemented in FreezeTagModeUtil.cpp:
        bool areAllOtherRunnersFrozen(PuppetInfo* player); // Only meant to be called on getting a packet, starts the endgame
        PlayerActorHakoniwa* getPlayerActorHakoniwa();     // Returns nullptr if the player is not a PlayerActorHakoniwa

        bool hasMarioCollision() override { return FreezeTagInfo::mHasMarioCollision; }
        bool hasMarioBounce()    override { return FreezeTagInfo::mHasMarioBounce;    }
        bool hasCappyCollision() override { return FreezeTagInfo::mHasCappyCollision; }
        bool hasCappyBounce()    override { return FreezeTagInfo::mHasCappyBounce;    }

    private:
        FreezeUpdateType         mNextUpdateType     = FreezeUpdateType::PLAYER;             // Set for the sendPacket funtion to know what packet type is sent
        FreezePostProcessingType mPostProcessingType = FreezePostProcessingType::PPDISABLED; // Current post processing mode (snapshot mode)

        GameModeTimer*  mModeTimer  = nullptr; // Generic timer from H&S used for round timer
        FreezeTagIcon*  mModeLayout = nullptr; // HUD layout (creates sub layout actors for runner and chaser)
        FreezeTagInfo*  mInfo       = nullptr; // Our own Freeze-Tag status
        al::WipeHolder* mWipeHolder = nullptr; // Pointer set by setWipeHolder on first step of hakoniwaSequence hook

        // Scene actors
        FreezePlayerBlock* mMainPlayerIceBlock = nullptr; // Visual block around player's when frozen
        FreezeHintArrow*   mHintArrow          = nullptr; // Arrow that points to nearest runner while being a chaser too far away from runners

        // Recovery event info (when falling off)
        int            mRecoveryEventFrames = 0;
        const int      mRecoveryEventLength = 60; // Length of recovery event in frames
        sead::Vector3f mRecoverySafetyPoint = sead::Vector3f::zero;

        // Endgame info (wipeout)
        bool  mIsEndgameActive  = false;
        bool  mCancelOnlyLegacy = false; // option to send a ROUNDCANCEL packet that only affects legacy clients
        float mEndgameTimer     = -1.0f; // timer to track how long wipeout is shown
        float mDisconnectTimer  =  0.0f; // timer to track how long to wait for a reconnect from disconnected players before cancelling the round  

        float mInvulnTime         = 0.0f;
        bool  mIsScoreEventsValid = false;

        // Spectate camera ticket and target information
        al::CameraTicket* mTicket            = nullptr;
        int               mPrevSpectateCount =  0;
        int               mPrevSpectateIndex = -2;
        int               mSpectateIndex     = -1;
};
