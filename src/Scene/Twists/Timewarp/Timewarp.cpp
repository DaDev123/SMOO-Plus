#include "Scene/Twists/Timewarp/Timewarp.hpp"

#include "hk/hook/Trampoline.h"
#include "hk/ro/RoUtil.h"

#include "sead/gfx/seadPrimitiveRenderer.h"

#include "al/Library/Base/StringUtil.h"
#include "al/Library/Controller/InputFunction.h"
#include "al/Library/Effect/EffectKeeper.h"
#include "al/Library/Effect/EffectSystemInfo.h"
#include "al/Library/LiveActor/ActorActionFunction.h"
#include "al/Library/LiveActor/ActorAnimFunction.h"
#include "al/Library/LiveActor/ActorMovementFunction.h"
#include "al/Library/LiveActor/ActorPoseKeeper.h"
#include "al/Library/LiveActor/ActorPoseUtil.h"
#include "al/Library/LiveActor/ActorSensorUtil.h"
#include "al/Library/Memory/HeapUtil.h"
#include "al/Library/Player/PlayerUtil.h"
#include "al/Library/Scene/SceneUtil.h"

#include "game/Player/HackCap.h"
#include "game/Player/PlayerAnimator.h"
#include "game/Player/PlayerFunction.h"
#include "game/Player/PlayerHackKeeper.h"
#include "game/Player/PlayerModelHolder.h"
#include "game/Player/PlayerOxygen.h"
#include "game/System/GameDataFunction.h"
#include "game/System/GameDataHolderAccessor.h"
#include "game/Util/ActorDimensionKeeper.h"
#include "game/Util/DemoUtil.h"

#include "math/seadMathCalcCommon.h"
#include "rs/util.hpp"

static HkTrampoline<bool> reduceOxygenForceHook = hk::hook::trampoline([]() -> bool {
    if (!TimeWarpTwist::sTimeWarpEnabled || !TimeWarpTwist::isOnCooldown())
        return reduceOxygenForceHook.orig();
    return TimeWarpTwist::isOnCooldown();
});

static HkTrampoline<void, PlayerOxygen*> oxygenReduceHook = hk::hook::trampoline([](PlayerOxygen* thisPtr) -> void {
    if (!TimeWarpTwist::sTimeWarpEnabled || !TimeWarpTwist::isOnCooldown()) {
        oxygenReduceHook.orig(thisPtr);
        return;
    }

    float oxygenRingCalc = TimeWarpTwist::calcCooldownPercent();

    if (thisPtr->mFramesReducing == 0)
        thisPtr->mFramesReducing = thisPtr->mOxygenNoReduceFrame;
    if (thisPtr->mFramesReducing >= thisPtr->mOxygenReduceFrame)
        thisPtr->mFramesReducing = thisPtr->mOxygenReduceFrame - 1;

    thisPtr->mOxygenLevel = oxygenRingCalc;
});

bool TimeWarpTwist::sTimeWarpEnabled = false;

bool TimeWarpTwist::sIsRewinding = false;
bool TimeWarpTwist::sIsCapture = false;
bool TimeWarpTwist::sIsCaptureInvalid = false;
bool TimeWarpTwist::sIs2D = false;
int TimeWarpTwist::sSceneInactiveTime = -1;

sead::PtrArray<TimeFrame> TimeWarpTwist::sTimeFrames;

int TimeWarpTwist::sRewindFrameDelay = 0;
int TimeWarpTwist::sRewindFrameDelayTarget = 0;

float TimeWarpTwist::sColorFrame = 0.f;
float TimeWarpTwist::sColorFrameOffset = 0.f;
int TimeWarpTwist::sDotBounceIndex = -1;

bool TimeWarpTwist::sIsCooldown = false;
float TimeWarpTwist::sCooldownCharge = 0.f;

StageScene* TimeWarpTwist::sStageScene = nullptr;

void TimeWarpTwist::init() {
    sTimeFrames.allocBuffer(maxFrames, nullptr);
    sSceneInactiveTime = 60;
    initHooks();
}

void TimeWarpTwist::initHooks() {
    reduceOxygenForceHook.installAtMainOffset(0x4855fc);
    oxygenReduceHook.installAtMainOffset(0x45ec88);
}

void TimeWarpTwist::toggleTimeWarp() {
    sTimeWarpEnabled = !sTimeWarpEnabled;
}

void TimeWarpTwist::onStageInit(StageScene* scene) {
    sStageScene = scene;
    emptyFrameInfo();
    sSceneInactiveTime = 60;
}

void TimeWarpTwist::onStageDeath() {
    sSceneInactiveTime = 25;
}

void TimeWarpTwist::update(PlayerActorHakoniwa* p1) {
    if (!sTimeWarpEnabled || !p1 || !sStageScene)
        return;

    al::LiveActor* hack = p1->mHackKeeper->mHackActor;
    bool isCur2D = p1->mDimensionKeeper->is2D();

    if (sSceneInactiveTime >= 0) {
        sSceneInactiveTime--;
        return;
    }

    if (sDotBounceIndex >= 0)
        sDotBounceIndex--;

    if (sIsCapture != (hack != nullptr)) {
        sIsCapture = hack != nullptr;
        sIsCaptureInvalid = false;
        emptyFrameInfo();
        if (sIsRewinding)
            endRewind(p1);
        if (sIsCapture && isInvalidCapture(p1->mHackKeeper->getCurrentHackName()))
            sIsCaptureInvalid = true;
    }

    if (sIs2D != isCur2D) {
        sIs2D = isCur2D;
        emptyFrameInfo();
        if (sIsRewinding)
            endRewind(p1);
    }

    if (sIsCooldown) {
        sCooldownCharge += cooldownRate;
        if (sCooldownCharge >= 100.f) {
            sIsCooldown = false;
            sDotBounceIndex = sTimeFrames.size() + 15;
            if (!p1->mDimensionKeeper->is2D())
                al::emitEffect(p1->mModelHolder->tryFindModelActor("Normal"), "DotReady", al::getTransPtr(p1));
        }
    }

    if (sIsCaptureInvalid)
        return;

    if (sTimeFrames.isEmpty()) {
        if (!rs::isActiveDemo(p1) && !sIsRewinding)
            pushNewFrame();
    } else {
        if ((al::calcDistance(p1, sTimeFrames.at(sTimeFrames.size() - 1)->position) > minPushDistance || p1->mHackCap->isFlying()) && !rs::isActiveDemo(p1) &&
            !PlayerFunction::isPlayerDeadStatus(p1) && !sIsRewinding)
            pushNewFrame();
    }

    if (al::isPadHoldR(-1) && !rs::isActiveDemo(p1) && !PlayerFunction::isPlayerDeadStatus(p1) && (sTimeFrames.size() >= minTrailLength || sIsRewinding) &&
        !sIsCooldown) {
        if (sRewindFrameDelay >= sRewindFrameDelayTarget)
            rewindFrame(p1);
        else
            sRewindFrameDelay++;
    } else if (sIsRewinding) {
        endRewind(p1);
    }
}

void TimeWarpTwist::drawTrail(al::Scene* scene, sead::PrimitiveRenderer* renderer) {
    if (!sTimeWarpEnabled || !scene || !isSceneActive() || getTimeArraySize() <= 1)
        return;

    renderer->begin();
    renderer->setModelMatrix(sead::Matrix34f::ident);

    for (int i = 0; i < sTimeFrames.size(); i++) {
        TimeFrame* frame = getTimeFrame(i);
        renderer->drawSphere4x8(calcDotTrans(frame->position, i), sead::MathCalcCommon<int>::min((sTimeFrames.size() - i) / 2, 9),
                                calcColorFrame(frame->colorFrame, i));
    }

    renderer->end();
}

void TimeWarpTwist::pushNewFrame() {
    sColorFrame += colorFrameRate;
    sDotBounceIndex--;

    sead::Heap* sceneHeap = al::getSceneHeap();
    TimeFrame* newFrame = sceneHeap ? new (sceneHeap) TimeFrame() : new TimeFrame();

    if (maxFrames <= sTimeFrames.size())
        delete sTimeFrames.popFront();

    al::PlayerHolder* pHolder = al::getScenePlayerHolder(sStageScene);
    PlayerActorHakoniwa* p1 = (PlayerActorHakoniwa*)al::tryGetPlayerActor(pHolder, 0);
    al::LiveActor* hack = p1->mHackKeeper->mHackActor;

    newFrame->colorFrame = sColorFrame;

    if (!hack) {
        newFrame->position = al::getTrans(p1);
        newFrame->playerFrame.gravity = al::getGravity(p1);
        newFrame->playerFrame.velocity = al::getVelocity(p1);
        newFrame->playerFrame.rotation = p1->mPoseKeeper->getQuat();
        newFrame->playerFrame.action.append(p1->mAnimator->mCurAnim.cstr());
        newFrame->playerFrame.actionFrame = p1->mAnimator->getAnimFrame();

        GameDataHolderAccessor accessor(sStageScene);
        if (GameDataFunction::isEnableCap(accessor) && p1->mHackCap) {
            newFrame->capFrame.isFlying = p1->mHackCap->isFlying();
            newFrame->capFrame.position = al::getTrans(p1->mHackCap);
            newFrame->capFrame.rotation = p1->mHackCap->mJointKeeper->mJointRot;
            newFrame->capFrame.action = al::getActionName(p1->mHackCap);
        }
    } else {
        newFrame->position = al::getTrans(hack);
        newFrame->playerFrame.velocity = al::getVelocity(hack);
        newFrame->playerFrame.rotation = hack->mPoseKeeper->getQuat();
        newFrame->playerFrame.action = al::getActionName(hack);
    }

    sTimeFrames.pushBack(newFrame);
}

void TimeWarpTwist::rewindFrame(PlayerActorHakoniwa* p1) {
    al::LiveActor* hack = p1->mHackKeeper->mHackActor;
    al::LiveActor* headModel = al::getSubActor(p1->mModelHolder->tryFindModelActor("Normal"), "頭");

    sRewindFrameDelay = 0;
    sColorFrame -= colorFrameRate;
    sColorFrameOffset += colorFrameOffsetRate;

    if (!sIsRewinding)
        startRewind(p1);

    if (sCooldownCharge > 0.f)
        sCooldownCharge -= cooldownDischarge;

    if (!hack) {
        al::setTrans(p1, sTimeFrames.back()->position);
        al::setGravity(p1, sTimeFrames.back()->playerFrame.gravity);
        al::setVelocity(p1, sTimeFrames.back()->playerFrame.velocity);
        al::setQuat(p1, sTimeFrames.back()->playerFrame.rotation);

        if (!sTimeFrames.back()->playerFrame.action.isEqual(p1->mAnimator->mCurAnim.cstr()))
            p1->mAnimator->startAnim(sTimeFrames.back()->playerFrame.action.cstr());
        p1->mAnimator->setAnimFrame(sTimeFrames.back()->playerFrame.actionFrame);

        GameDataHolderAccessor accessor(sStageScene);
        if (GameDataFunction::isEnableCap(accessor) && !p1->mDimensionKeeper->is2D() && p1->mHackCap) {
            updateHackCap(p1->mHackCap, headModel);
            al::setTrans(p1->mHackCap, sTimeFrames.back()->capFrame.position);
            p1->mHackCap->mJointKeeper->mJointRot = sTimeFrames.back()->capFrame.rotation;
            al::startAction(p1->mHackCap, sTimeFrames.back()->capFrame.action.cstr());
        }
    } else {
        al::setTrans(hack, sTimeFrames.back()->position);
        al::setVelocity(hack, sTimeFrames.back()->playerFrame.velocity);
        al::setQuat(hack, sTimeFrames.back()->playerFrame.rotation);
        al::startAction(hack, sTimeFrames.back()->playerFrame.action.cstr());
    }

    if (!p1->mDimensionKeeper->is2D())
        al::emitEffect(p1->mModelHolder->tryFindModelActor("Normal"), "DotTravel", al::getTransPtr(p1));

    delete sTimeFrames.popBack();

    if (sTimeFrames.isEmpty())
        endRewind(p1);
}

void TimeWarpTwist::updateHackCap(HackCap* cap, al::LiveActor* headModel) {
    if (sTimeFrames.back()->capFrame.isFlying != cap->isFlying()) {
        if (sTimeFrames.back()->capFrame.isFlying)
            cap->setupThrowStart();
        else {
            cap->startCatch("Default", true, al::getTrans(cap));
            cap->forcePutOn();
        }
    }

    if (sTimeFrames.back()->capFrame.isFlying) {
        cap->showPuppetCap();
        al::startVisAnimForAction(headModel, "CapOff");
    } else {
        cap->hidePuppetCap();
        al::startVisAnimForAction(headModel, "CapOn");
    }
}

void TimeWarpTwist::startRewind(PlayerActorHakoniwa* p1) {
    if (!p1->mHackKeeper->mHackActor) {
        p1->startDemoPuppetable();
        p1->mAnimator->startAnim("Default");
    }
    sIsRewinding = true;
    sCooldownCharge = 60.f;
    setPostProcessingId(4);
}

void TimeWarpTwist::endRewind(PlayerActorHakoniwa* p1) {
    if (!p1->mHackKeeper->mHackActor) {
        p1->endDemoPuppetable();
        p1->mHackCap->startCatch("Default", true, al::getTrans(p1));
        p1->mHackCap->forcePutOn();
        p1->mHackCap->hidePuppetCap();
    }

    GameDataHolderAccessor accessor(sStageScene);
    if (GameDataFunction::isEnableCap(accessor) && p1->mHackCap)
        p1->mHackCap->mCapActionHistory->mIsCapJumpPossible = false;

    sIsRewinding = false;
    resetCooldown();
    setPostProcessingId(0);
}

void TimeWarpTwist::setPostProcessingId(int targetId) {
    if (!sStageScene)
        return;

    u32 curId = al::getPostProcessingFilterPresetId(sStageScene);

    if (targetId == 0) {
        while (curId != 0) {
            al::incrementPostProcessingFilterPreset(sStageScene);
            curId = (curId + 1) % 18;
        }
        al::invalidatePostProcessingFilter(sStageScene);
        return;
    }

    while (curId != (u32)targetId) {
        al::incrementPostProcessingFilterPreset(sStageScene);
        curId = (curId + 1) % 18;
    }
    al::validatePostProcessingFilter(sStageScene);
}

void TimeWarpTwist::emptyFrameInfo() {
    sTimeFrames.clear();
    sColorFrame = 0.f;
}

void TimeWarpTwist::resetCooldown() {
    sIsCooldown = true;
    sColorFrameOffset = 0.f;
}

bool TimeWarpTwist::isInvalidCapture(const char* curName) {
    constexpr static const char* hackList[] = {"ElectricWire",
                                               "TRex",
                                               "Fukankun",
                                               "Cactus",
                                               "BazookaElectric",
                                               "JugemFishing",
                                               "Fastener",
                                               "GotogotonLake",
                                               "GotogotonCity",
                                               "Senobi",
                                               "Tree",
                                               "RockForest",
                                               "FukuwaraiFacePartsKuribo",
                                               "Imomu",
                                               "AnagramAlphabetCharacter",
                                               "Car",
                                               "Manhole",
                                               "Tsukkun",
                                               "Statue",
                                               "StatueKoopa",
                                               "KaronWing",
                                               "Bull",
                                               "Koopa",
                                               "Yoshi"};
    constexpr int hackListSize = sizeof(hackList) / sizeof(hackList[0]);
    for (int i = 0; i < hackListSize; i++) {
        if (al::isEqualString(curName, hackList[i]))
            return true;
    }
    return false;
}

int TimeWarpTwist::getTimeArraySize() {
    return sTimeFrames.size();
}
float TimeWarpTwist::getColorFrame() {
    return sColorFrame;
}
float TimeWarpTwist::getCooldownTimer() {
    return sCooldownCharge;
}
int TimeWarpTwist::getRewindDelay() {
    return sRewindFrameDelayTarget;
}
bool TimeWarpTwist::isSceneActive() {
    return sSceneInactiveTime == -1;
}
bool TimeWarpTwist::isRewind() {
    return sIsRewinding;
}
bool TimeWarpTwist::isOnCooldown() {
    return sIsCooldown;
}

TimeFrame* TimeWarpTwist::getTimeFrame(u32 index) {
    if (sTimeFrames.isEmpty())
        return nullptr;
    if (index > (u32)(sTimeFrames.size() - 1))
        return sTimeFrames.at(sTimeFrames.size() - 1);
    return sTimeFrames.at(index);
}

sead::Color4f TimeWarpTwist::calcColorFrame(float frame, int dotIndex) {
    sead::Color4f color = {0.f, 0.f, 0.f, 0.7f};
    if (sIsCooldown) {
        float v = ((100 + sCooldownCharge) / 100) - 1;
        color.r = v;
        color.g = v;
        color.b = v;
    } else {
        if (dotIndex < sDotBounceIndex - 8) {
            color.r = 1.f;
            color.g = 1.f;
            color.b = 1.f;
        } else {
            color.r = sin(frame + sColorFrameOffset);
            color.g = sin(frame + sColorFrameOffset - 2.0942f);
            color.b = sin(frame + sColorFrameOffset - 4.1884f);
        }
    }
    return color;
}

sead::Vector3f TimeWarpTwist::calcDotTrans(sead::Vector3f position, int dotIndex) {
    int relative = sDotBounceIndex - dotIndex;
    if (relative > 0 && relative <= 15)
        position.y += sin(0.21f * relative) * 80.f;
    return position;
}

float TimeWarpTwist::calcCooldownPercent() {
    return sead::MathCalcCommon<float>::max(sead::MathCalcCommon<float>::min(sCooldownCharge / 100.f, 1.f), 0.05f);
}