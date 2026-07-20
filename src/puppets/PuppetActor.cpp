#include "actors/PuppetActor.h"

#include "hk/diag/diag.h"

#include "sead/heap/seadHeapMgr.h"
#include "sead/math/seadQuat.h"

#include "al/Library/Action/ActorActionKeeper.h"
#include "al/Library/Base/StringUtil.h"
#include "al/Library/Draw/GraphicsSystemInfo.h"
#include "al/Library/Effect/EffectSystemInfo.h"
#include "al/Library/HitSensor/HitSensorKeeper.h"
#include "al/Library/Light/ModelMaterialCategory.h"
#include "al/Library/LiveActor/ActorActionFunction.h"
#include "al/Library/LiveActor/ActorAnimFunction.h"
#include "al/Library/LiveActor/ActorClippingFunction.h"
#include "al/Library/LiveActor/ActorFlagFunction.h"
#include "al/Library/LiveActor/ActorInitFunction.h"
#include "al/Library/LiveActor/ActorInitUtil.h"
#include "al/Library/LiveActor/ActorModelFunction.h"
#include "al/Library/LiveActor/ActorPoseKeeper.h"
#include "al/Library/LiveActor/ActorPoseUtil.h"
#include "al/Library/LiveActor/ActorResourceFunction.h"
#include "al/Library/LiveActor/ActorSensorUtil.h"
#include "al/Library/LiveActor/LiveActor.h"
#include "al/Library/LiveActor/LiveActorFunction.h"
#include "al/Library/LiveActor/LiveActorKeeper.h"
#include "al/Library/Memory/HeapUtil.h"
#include "al/Library/Nerve/NerveUtil.h"
#include "al/Library/Obj/PartsModel.h"
#include "al/Library/Play/Layout/BalloonMessage.h"
#include "al/Library/Resource/ActorResource.h"
#include "al/Library/Resource/ResourceFunction.h"
#include "al/Library/Yaml/ByamlIter.h"
#include "al/Project/Action/ActionPadAndCameraCtrl.h"
#include "al/Project/HitSensor/HitSensor.h"

#include "game/Player/PlayerCostumeFunction.h"
#include "game/Player/PlayerCostumeInfo.h"
#include "game/Player/PlayerFunction.h"
#include "game/Util/PlayerUtil.h"
#include "game/Util/SensorMsgFunction.h"

#include <cstddef>

#include "algorithms/CaptureTypes.h"
#include "helpers.hpp"
#include "Scene/StageSceneStateModConfig.hpp"
#include "server/DeltaTime.hpp"

static const char* subActorNames[] = {
    "顔",    // Face
    "目",    // Eye
    "頭",    // Head
    "左手",  // Left Hand
    "右手"   // Right Hand
};

PuppetActor::PuppetActor(const char* name) : al::LiveActor(name) {
    sead::ScopedCurrentHeapSetter setter(al::getSceneHeap());

    mPuppetCap = new PuppetCapActor(name);
    mCaptures = new HackModelHolder();
    mModelHolder = new PlayerModelHolder(3);  // Regular Model, 2D Model, 2D Mini Model
    // what is a 2d mini model? i feel like we could get rid of that but w/e
}

PuppetActor::~PuppetActor() {
    delete mCostumeInfo;
    mInfo = nullptr;
    delete mPuppetCap;
    delete mModelHolder;
    delete mCaptures;
    delete mNameTag;
}

void PuppetActor::init(al::ActorInitInfo const& initInfo) {
    sead::ScopedCurrentHeapSetter setter(al::getSceneHeap());

    mPuppetCap->init(initInfo);
    al::initActorWithArchiveName(this, initInfo, "PlayerActorHakoniwa", nullptr);

    const char* bodyName = "Mario";
    const char* capName = "Mario";

    if (mInfo) {
        bodyName = tryGetPuppetBodyName(mInfo);
        capName = tryGetPuppetCapName(mInfo);

        mNameTag =
            new NameTag(this, al::getLayoutInitInfo(initInfo), 4900.0f, 5000.0f, mInfo->puppetName.cstr());
    }

    al::LiveActor* normalModel = new al::LiveActor("Normal");

    mCostumeInfo = initMarioModelPuppet(normalModel, initInfo, bodyName, capName, 0, nullptr);

    normalModel->mActionKeeper->mPadAndCameraCtrl->mRumbleCount = 0;

    mModelHolder->registerModel(normalModel, "Normal");

    al::LiveActor* normal2DModel = new al::LiveActor("Normal2D");

    PlayerFunction::initMarioModelActor2D(
        normal2DModel, initInfo, al::StringTmp<0x40>("%s2D", mCostumeInfo->mBodyInfo->costumeName).cstr(),
        PlayerFunction::isInvisibleCap(mCostumeInfo));

    mModelHolder->registerModel(normal2DModel, "Normal2D");

    al::setClippingInfo(normalModel, 999999999.0f, 0);
    al::setClippingNearDistance(normalModel, 999999999.0f);

    al::setClippingInfo(normal2DModel, 999999999.0f, 0);
    al::setClippingNearDistance(normal2DModel, 999999999.0f);

    al::hideSilhouetteModelIfShow(normalModel);

    al::LiveActor* headModel = al::getSubActor(normalModel, "頭");
    al::getSubActor(headModel, "キャップの目")->kill();
    al::startVisAnimForAction(headModel, "CapOn");

    mModelHolder->changeModel("Normal");

    startAction("Wait");

    // Clear existing sensors loaded from BYML
    if (mHitSensorKeeper) {
        mHitSensorKeeper->clear();
    }

    initHitSensor(3);
    al::addHitSensor(this, initInfo, "Body", static_cast<u32>(al::HitSensorType::Npc), 50.0f, 16,
                     sead::Vector3f(0.0f, 75.0f, 0.0f));
    al::addHitSensor(this, initInfo, "Head", static_cast<u32>(al::HitSensorType::Npc), 40.0f, 16,
                     sead::Vector3f(0.0f, 110.0f, 0.0f));
    al::addHitSensor(this, initInfo, "Foot", static_cast<u32>(al::HitSensorType::Npc), 40.0f, 1,
                     sead::Vector3f(0.0f, 40.0f, 0.0f));

    al::validateClipping(normalModel);
    al::validateClipping(normal2DModel);
}

void PuppetActor::initAfterPlacement() {
    al::LiveActor::initAfterPlacement();
}

void PuppetActor::initOnline(PuppetInfo* pupInfo) {
    mInfo = pupInfo;

    mPuppetCap->initOnline(pupInfo);
}

void PuppetActor::movement() {
    al::LiveActor::movement();
}

void PuppetActor::calcAnim() {
    al::LiveActor::calcAnim();
}

void PuppetActor::control() {
    if (mInfo) {
        al::LiveActor* curModel = getCurrentModel();

        // Animation Updating

        if (!al::isActionPlaying(curModel, mInfo->curSubAnimStr.cstr())) {
            startAction(mInfo->curAnimStr.cstr());
        } else if (al::isActionEnd(curModel)) {
            startAction(mInfo->curAnimStr.cstr());
        }

        if (isNeedBlending()) {
            for (size_t i = 0; i < 6; i++) {
                setBlendWeight(i, mInfo->blendWeights[i]);
            }
        }

        // Position & Rotation Handling

        // Use smooth movement if low latency is disabled, otherwise snap directly
        if (!StageSceneStateModConfig::isLowLatencyEnabled()) {
            sead::Vector3f* pPos = al::getTransPtr(this);
            sead::Quatf* pQuat = al::getQuatPtr(this);

            mClosingSpeed = VisualUtils::SmoothMove({pPos, pQuat}, {&mInfo->playerPos, &mInfo->playerRot},
                                                    Time::deltaTime, mClosingSpeed, 1440.0f);
        } else {
            al::setTrans(this, mInfo->playerPos);
            al::setQuat(this, mInfo->playerRot);
        }

        // Model Updating

        if (!mIs2DModel && mInfo->is2D) {
            changeModel("Normal2D");
            mIs2DModel = true;

        } else if (mIs2DModel && !mInfo->is2D) {
            changeModel("Normal");
            mIs2DModel = false;
        }

        // Capture Updating

        if (mInfo->isCaptured && !mIsCaptureModel) {
            getCurrentModel()->makeActorDead();  // sets previous model to dead so we can try to
                                                 // switch to capture model
            setCapture(mInfo->curHack.cstr());
            mIsCaptureModel = true;
            getCurrentModel()->makeActorAlive();  // make new model alive

        } else if (!mInfo->isCaptured && mIsCaptureModel) {
            getCurrentModel()->makeActorDead();   // make capture model dead
            mModelHolder->changeModel("Normal");  // set player model to normal
            mIsCaptureModel = false;
            getCurrentModel()->makeActorAlive();  // make player model alive
        }

        // Visibility Updating

        if (mInfo->isCapThrow) {
            if (al::isDead(mPuppetCap)) {
                mPuppetCap->makeActorAlive();
                al::setTrans(mPuppetCap, mInfo->capPos);
            }
        } else {
            if (al::isAlive(mPuppetCap)) {
                mPuppetCap->makeActorDead();

                // startAction(mInfo->curSubAnimStr);

                al::LiveActor* headModel = al::getSubActor(curModel, "頭");
                if (headModel) {
                    al::startVisAnimForAction(headModel, "CapOn");
                }
            }
        }

        if (mNameTag) {
            if (!mNameTag->mIsAlive)
                mNameTag->appear();
            mNameTag->mIsAlive = true;
        }

        // Sub-Actor Updating

        mPuppetCap->update();

        // Syncing

        syncPose();
    }
}

void PuppetActor::setBlendWeight(int index, float weight) {
    al::LiveActor* curModel = getCurrentModel();
    if (curModel && curModel->mActionKeeper) {
        al::setSklAnimBlendWeight(curModel, weight, index);
    }
}

void PuppetActor::makeActorAlive() {
    al::LiveActor* curModel = getCurrentModel();

    if (al::isDead(curModel)) {
        curModel->makeActorAlive();
    }

    // update name tag when puppet becomes active again
    if (mInfo) {
        if (mNameTag) {
            mNameTag->setText(mInfo->puppetName.cstr());
        }
    }

    al::LiveActor::makeActorAlive();
}

void PuppetActor::makeActorDead() {
    al::LiveActor* curModel = getCurrentModel();

    if (!al::isDead(curModel)) {
        curModel->makeActorDead();
    }

    mPuppetCap->makeActorDead();

    al::LiveActor::makeActorDead();
}

void PuppetActor::attackSensor(al::HitSensor* source, al::HitSensor* target) {
    if (!StageSceneStateModConfig::isPuppetCollisionEnabled()) {
        return;
    }

    if (!al::sendMsgPush(target, source)) {
        rs::sendMsgPushToPlayer(target, source);
    }
}

bool PuppetActor::receiveMsg(const al::SensorMsg* msg, al::HitSensor* source, al::HitSensor* target) {
    if (!StageSceneStateModConfig::isPuppetBounceEnabled()) {
        return false;
    }

    if ((al::isMsgPlayerTrampleReflect(msg) || rs::isMsgPlayerAndCapObjHipDropReflectAll(msg)) &&
        al::isSensorName(target, "Body")) {
        rs::requestHitReactionToAttacker(msg, target, source);
        return true;
    }

    return false;
}

void PuppetActor::startAction(const char* actName) {
    al::LiveActor* curModel = getCurrentModel();

    if (!actName)
        return;

    if (al::tryStartActionIfNotPlaying(curModel, actName)) {
        const char* curActName = al::getActionName(curModel);
        if (curActName) {
            if (al::isSklAnimExist(curModel, curActName)) {
                al::clearSklAnimInterpole(curModel);
            }
        }
    }

    for (size_t i = 0; i < 5; i++) {
        al::LiveActor* subActor = al::getSubActor(curModel, subActorNames[i]);
        const char* curActName = al::getActionName(curModel);
        if (subActor && curActName) {
            if (al::tryStartActionIfNotPlaying(subActor, curActName)) {
                if (al::isSklAnimExist(curModel, curActName)) {
                    al::clearSklAnimInterpole(curModel);
                }
            }
        }
    }

    al::LiveActor* faceActor = al::tryGetSubActor(curModel, "顔");

    if (faceActor) {
        al::StringTmp<0x80> faceAnim("%sFullFace", actName);
        if (al::tryStartActionIfNotPlaying(faceActor, faceAnim.cstr())) {
            if (al::isSklAnimExist(faceActor, faceAnim.cstr())) {
                al::clearSklAnimInterpole(faceActor);
            }
        }
    }
}

void PuppetActor::hairControl() {
    al::LiveActor* curModel = getCurrentModel();

    if (mCostumeInfo->isNeedSyncBodyHair()) {
        PlayerFunction::syncBodyHairVisibility(al::getSubActor(curModel, "髪"),
                                               al::getSubActor(curModel, "頭"));
    }
    if (mCostumeInfo->isSyncFaceBeard()) {
        PlayerFunction::syncMarioFaceBeardVisibility(al::getSubActor(curModel, "顔"),
                                                     al::getSubActor(curModel, "頭"));
    }
    if (mCostumeInfo->isSyncStrap()) {
        PlayerFunction::syncMarioHeadStrapVisibility(al::getSubActor(curModel, "頭"));
    }
    if (PlayerFunction::isNeedHairControl(mCostumeInfo->mBodyInfo, mCostumeInfo->mHeadInfo->costumeName)) {
        PlayerFunction::hideHairVisibility(al::getSubActor(curModel, "頭"));
    }
}

bool PuppetActor::isNeedBlending() {
    const char* curActName = al::getActionName(getCurrentModel());
    if (curActName) {
        return al::isEqualSubString(curActName, "Move") || al::isEqualSubString(curActName, "Sand") ||
               al::isEqualSubString(curActName, "MotorcycleRide");
    } else {
        return false;
    }
}

bool PuppetActor::isInCaptureList(const char* hackName) {
    return mCaptures->getCapture(hackName) != nullptr;
}

bool PuppetActor::addCapture(PuppetHackActor* capture, const char* hackType) {
    if (mCaptures->addCapture(capture, hackType)) {
        return true;
    }

    return false;
}

void PuppetActor::changeModel(const char* newModel) {
    getCurrentModel()->makeActorDead();
    mModelHolder->changeModel(newModel);
    getCurrentModel()->makeActorAlive();
}

al::LiveActor* PuppetActor::getCurrentModel() {
    if (mIsCaptureModel) {
        al::LiveActor* curCapture = mCaptures->getCurrentActor();
        if (curCapture) {
            return curCapture;
        }
    }
    return mModelHolder->mCurrentModel->actor;
}

void PuppetActor::debugTeleportCaptures(const sead::Vector3f& pos) {
    for (int i = 0; i < mCaptures->getEntryCount(); i++) {
        al::LiveActor* capture = mCaptures->getCapture(i);
        if (capture) {
            al::setTrans(capture, al::getTrans(getCurrentModel()));
        }
    }
}

void PuppetActor::debugTeleportCapture(const sead::Vector3f& pos, int index) {
    al::LiveActor* capture = mCaptures->getCapture(index);
    if (capture) {
        al::setTrans(capture, al::getTrans(getCurrentModel()));
    }
}

bool PuppetActor::setCapture(const char* captureName) {
    if (captureName && mCaptures->setCurrent(captureName)) {
        mCurCapture = CaptureTypes::FindType(captureName);
        return true;
    } else {
        mCurCapture = CaptureTypes::Type::Unknown;
        return false;
    }
}

void PuppetActor::syncPose() {
    al::LiveActor* curModel = getCurrentModel();

    curModel->mPoseKeeper->updatePoseQuat(
        al::getQuat(this));  // update pose using a quaternion instead of setting quaternion rotation

    al::setTrans(curModel, al::getTrans(this));
}

void PuppetActor::emitJoinEffect() {
    al::tryDeleteEffect(this, "Disappear");  // remove previous effect (if played previously)

    al::tryEmitEffect(this, "Disappear", nullptr);
}

const char* executorName = "ＮＰＣ";

PlayerCostumeInfo* initMarioModelPuppet(al::LiveActor* player, const al::ActorInitInfo& initInfo,
                                        const char* bodyName, const char* capName, int subActorNum,
                                        al::AudioKeeper* audioKeeper) {
    sead::ScopedCurrentHeapSetter setter(al::getSceneHeap());

    // hk::diag::logLine("Loading Resources for Mario Puppet Model.");

    al::ActorResource* modelRes = al::findOrCreateActorResourceWithAnimResource(
        initInfo.actorResourceHolder, al::StringTmp<0x100>("ObjectData/%s", bodyName).cstr(),
        al::StringTmp<0x100>("ObjectData/%s", "PlayerAnimation").cstr(), 0, false);

    // hk::diag::logLine("Creating Body Costume Info.");

    PlayerBodyCostumeInfo* bodyInfo =
        PlayerCostumeFunction::createBodyCostumeInfo(modelRes->mModelRes, bodyName);

    // hk::diag::logLine("Initializing Basic Actor Data.");

    al::initActorSceneInfo(player, initInfo);
    al::initActorPoseTQGSV(player);
    al::initActorSRT(player, initInfo);

    al::initActorModelKeeper(player, initInfo, al::StringTmp<0x100>("ObjectData/%s", bodyName).cstr(), 6,
                             al::StringTmp<0x100>("ObjectData/%s", "PlayerAnimation").cstr());

    // hk::diag::logLine("Creating Material Category for Player Type");

    al::ModelMaterialCategory::tryCreate(player->mModelKeeper->mModelCtrl, "Player",
                                         initInfo.actorSceneInfo.graphicsSystemInfo->mMaterialCategoryKeeper);

    // hk::diag::logLine("Initing Skeleton.");

    al::initPartialSklAnim(player, 1, 1, 32);
    al::addPartialSklAnimPartsListRecursive(player, "Spine1", 0);

    // hk::diag::logLine("Setting Up Executor Info.");

    al::initExecutorUpdate(player, initInfo, executorName);
    al::initExecutorDraw(player, initInfo, executorName);
    al::initExecutorModelUpdate(player, initInfo);

    // hk::diag::logLine("Getting InitEffect Byml from resource.");

    al::ByamlIter iter;
    if (al::tryGetActorInitFileIter(&iter, modelRes->mModelRes, "InitEffect", 0)) {
        const char* effectKeeperName;
        if (iter.tryGetStringByKey(&effectKeeperName, "Name")) {
            // hk::diag::logLine("Initializing Effect Keeper.");

            al::initActorEffectKeeper(player, initInfo, effectKeeperName);
        }
    }

    // hk::diag::logLine("Initing Player Audio.");

    PlayerFunction::initMarioAudio(player, initInfo, modelRes->mModelRes, false, audioKeeper);
    al::initActorActionKeeper(player, modelRes, bodyName, 0);
    al::setMaterialProgrammable(player);

    // hk::diag::logLine("Creating Sub-Actor Keeper.");

    al::SubActorKeeper* actorKeeper = al::SubActorKeeper::tryCreate(player, 0, subActorNum);

    if (actorKeeper) {
        player->initSubActorKeeper(actorKeeper);
    }

    actorKeeper->init(initInfo, 0, subActorNum);

    // hk::diag::logLine("Initializing Sub-Actors.");

    int subModelNum = al::getSubActorNum(player);

    if (subModelNum >= 1) {
        for (int i = 0; i < subModelNum; i++) {
            al::LiveActor* subActor = al::getSubActor(player, i);
            const char* actorName = subActor->getName();

            if (!al::isEqualString(actorName, "シルエットモデル")) {
                al::initExecutorUpdate(subActor, initInfo, executorName);
                al::initExecutorDraw(subActor, initInfo, executorName);
                al::setMaterialProgrammable(subActor);
            }
        }
    }

    // hk::diag::logLine("Creating Clipping.");

    al::initActorClipping(player, initInfo);
    al::invalidateClipping(player);

    // hk::diag::logLine("Getting Cap Model/Head Model Name.");

    const char* capModelName;

    if (bodyInfo->isUseHeadSuffix) {
        if (al::isEqualString(bodyInfo->costumeName, capName)) {
            capModelName = "";
        } else {
            capModelName = capName;
        }
    } else {
        capModelName = "";
    }

    const char* headType;

    if (!al::isEqualSubString(capName, "Mario64")) {
        if (bodyInfo->isUseShortHead && al::isEqualString(capName, "MarioPeach")) {
            headType = "Short";
        } else {
            headType = "";
        }
    } else if (al::isEqualString(bodyInfo->costumeName, "Mario64")) {
        headType = "";
    } else if (al::isEqualString(bodyInfo->costumeName, "Mario64Metal")) {
        headType = "Metal";
    } else {
        headType = "Other";
    }

    // hk::diag::logLine("Creating Head Costume Info. Cap Model: %s. Head Type: %s. Cap Name: %s.",
    // capModelName, headType, capName);

    PlayerHeadCostumeInfo* headInfo =
        initMarioHeadCostumeInfo(player, initInfo, "頭", capName, headType, capModelName);

    // hk::diag::logLine("Creating Costume Info.");

    PlayerCostumeInfo* costumeInfo = new PlayerCostumeInfo();
    costumeInfo->init(bodyInfo, headInfo);

    if (costumeInfo->isNeedBodyHair()) {
        hk::diag::logLine("Creating Body Hair Parts Model.");

        al::PartsModel* partsModel = new al::PartsModel("髪");

        partsModel->initPartsFixFile(
            player, initInfo,
            al::StringTmp<0x100>("%sHair%s", bodyName, costumeInfo->isEnableHairNoCap() ? "NoCap" : "")
                .cstr(),
            0, "Hair");

        al::initExecutorUpdate(partsModel, initInfo, executorName);
        al::initExecutorDraw(partsModel, initInfo, executorName);
        al::setMaterialProgrammable(partsModel);
        partsModel->makeActorDead();
        al::onSyncAppearSubActor(player, partsModel);
        al::onSyncClippingSubActor(player, partsModel);
        al::onSyncAlphaMaskSubActor(player, partsModel);
        al::onSyncHideSubActor(player, partsModel);
    }

    // hk::diag::logLine("Initing Depth Model.");

    PlayerFunction::initMarioDepthModel(player, false, false);

    // hk::diag::logLine("Creating Retarget Info.");

    rs::createPlayerSklRetargettingInfo(player, sead::Vector3f::ones);

    // hk::diag::logLine("Making Player Model Dead.");

    player->makeActorDead();
    return costumeInfo;
}

PlayerHeadCostumeInfo* initMarioHeadCostumeInfo(al::LiveActor* player, const al::ActorInitInfo& initInfo,
                                                const char* headModelName, const char* capModelName,
                                                const char* headType, const char* headSuffix) {
    sead::ScopedCurrentHeapSetter setter(al::getSceneHeap());

    al::PartsModel* headModel = new al::PartsModel(headModelName);

    al::StringTmp<0x80> headArcName("%sHead%s", capModelName, headType);
    al::StringTmp<0x100> arcSuffix("Head");
    if (headSuffix)
        arcSuffix.format("Head%s", headSuffix);

    headModel->initPartsFixFile(player, initInfo, headArcName.cstr(), 0, arcSuffix.cstr());
    al::setMaterialProgrammable(headModel);

    al::initExecutorUpdate(headModel, initInfo, executorName);
    al::initExecutorDraw(headModel, initInfo, executorName);

    headModel->makeActorDead();

    al::onSyncAppearSubActor(player, headModel);
    al::onSyncClippingSubActor(player, headModel);
    al::onSyncAlphaMaskSubActor(player, headModel);
    al::onSyncHideSubActor(player, headModel);

    al::PartsModel* capEyesModel = new al::PartsModel("キャップの目");
    capEyesModel->initPartsFixFile(headModel, initInfo, "CapManHeroEyes", "", 0);

    al::onSyncClippingSubActor(headModel, capEyesModel);
    al::onSyncAlphaMaskSubActor(headModel, capEyesModel);
    al::onSyncHideSubActor(headModel, capEyesModel);

    al::setMaterialProgrammable(headModel);
    headModel->makeActorDead();

    return PlayerCostumeFunction::createHeadCostumeInfo(al::getModelResource(headModel), capModelName, false);
}