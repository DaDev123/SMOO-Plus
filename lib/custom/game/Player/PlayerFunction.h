#pragma once

#include "sead/math/seadMatrix.h"
#include "sead/prim/seadSafeString.h"

namespace al {
class LiveActor;
struct ActorInitInfo;
class Resource;
class AudioKeeper;
}  // namespace al

class PlayerConst;
class PlayerCostumeInfo;
struct PlayerBodyCostumeInfo;
struct PlayerHeadCostumeInfo;
class PlayerJointControlPartsDynamics;

namespace PlayerFunction {
u32 getPlayerInputPort(const al::LiveActor*);
const sead::Matrix34f& getPlayerViewMtx(const al::LiveActor*);
bool tryActivateAmiiboPreventDamage(const al::LiveActor*);
bool isPlayerDeadStatus(const al::LiveActor* actor);
void syncBodyHairVisibility(al::LiveActor*, al::LiveActor*);
void syncMarioFaceBeardVisibility(al::LiveActor*, al::LiveActor*);
void syncMarioHeadStrapVisibility(al::LiveActor*);
bool isNeedHairControl(const PlayerBodyCostumeInfo*, const char*);
bool isInvisibleCap(const PlayerCostumeInfo*);
void hideHairVisibility(al::LiveActor*);

PlayerConst* createMarioConst(const char*);
void createCapModelName(sead::BufferedSafeString*, const char*);

void initMarioModelActor2D(al::LiveActor* actor, const al::ActorInitInfo& initInfo, const char* model2DName,
                           bool isInvisCap);
al::Resource* initCapModelActor(al::LiveActor*, const al::ActorInitInfo&, const char*);
al::Resource* initCapModelActorDemo(al::LiveActor*, const al::ActorInitInfo&, const char*);
PlayerCostumeInfo* initMarioModelActor(al::LiveActor* player, const al::ActorInitInfo& initInfo,
                                       const char* modelName, const char* capType, al::AudioKeeper* keeper,
                                       bool isCloset);
PlayerCostumeInfo* initMarioModelActorDemo(PlayerJointControlPartsDynamics** jointCtrlPtr,
                                           al::LiveActor* player, const al::ActorInitInfo& initInfo,
                                           const char* bodyName, const char* capName,
                                           const PlayerConst* pConst, sead::Vector3f* noseScale,
                                           sead::Vector3f* earScale, bool isCloset);
PlayerCostumeInfo* initMarioModelCommon(al::LiveActor* player, const al::ActorInitInfo& initInfo,
                                        const char* bodyName, const char* capName, int subActorNum,
                                        bool isDemo, al::AudioKeeper* audioKeeper, bool guessIsChromaKey,
                                        bool isCloset);
// not a real symbol, func at 0x445A24
void initMarioAudio(al::LiveActor* player, const al::ActorInitInfo& initInfo, al::Resource* modelRes,
                    bool isDemo, al::AudioKeeper* audioKeeper);
// not a real symbol, func at 0x448B8C
void initMarioSubModel(al::LiveActor* subactor, const al::ActorInitInfo& initInfo, bool isInvisible,
                       bool isDemo, bool isChromaKey, bool isCloset);
// not a real symbol, func at 0x445128
PlayerHeadCostumeInfo* initMarioHeadCostumeInfo(al::LiveActor* player, const al::ActorInitInfo& initInfo,
                                                const char*, const char*, const char*, const char*, bool,
                                                bool, bool, bool, bool);
// not a real symbol, func at 0x445DF4
void initMarioDepthModel(al::LiveActor* player, bool isDemo, bool isChromaKey);
};  // namespace PlayerFunction
