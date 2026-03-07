#include "server/shine-thief/ShineThiefPlayerBlock.h"

#include "al/Library/Camera/CameraDirector.h"
#include "al/Library/Camera/CameraPoser.h"
#include "al/Library/Camera/CameraPoseUpdater.h"
#include "al/Library/Effect/EffectSystemInfo.h"
#include "al/Library/LiveActor/ActorActionFunction.h"
#include "al/Library/LiveActor/ActorClippingFunction.h"
#include "al/Library/LiveActor/ActorCollisionFunction.h"
#include "al/Library/LiveActor/ActorFlagFunction.h"
#include "al/Library/LiveActor/ActorInitUtil.h"
#include "al/Library/LiveActor/ActorModelFunction.h"
#include "al/Library/LiveActor/ActorMovementFunction.h"
#include "al/Library/LiveActor/ActorPoseUtil.h"
#include "al/Library/LiveActor/ActorSensorUtil.h"
#include "al/Library/LiveActor/LiveActor.h"
#include "al/Library/Math/MathUtil.h"
#include "al/Library/Nerve/NerveUtil.h"

#include "game/Util/ActorDimensionKeeper.h"

#include "../src/Scene/Twists/SmallMario/smallMarioHooks.hpp"
#include "Library/Camera/CameraTicket.h"
#include "Library/LiveActor/ActorSceneInfo.h"
#include "Scene/Twists/TwistsConfig.hpp"
#include "server/DeltaTime.hpp"
#include "server/gamemode/GameModeManager.hpp"
#include "server/shine-thief/ShineThiefInfo.h"
#include "server/shine-thief/ShineThiefMode.hpp"

static float getBlockTargetScale() {
    return TwistsConfig::isSmallMarioEnabled() ? 0.6f * ::scale : 0.6f;
}

ShineThiefPlayerBlock::ShineThiefPlayerBlock(const char* name) : al::LiveActor(name) {}

void ShineThiefPlayerBlock::init(al::ActorInitInfo const& info) {
    al::initActorWithArchiveName(this, info, "ShineThiefPlayerBlock", nullptr);
    al::initNerve(this, &NrvShineThiefPlayerBlock.Appear, 0);

    al::hideSilhouetteModelIfShow(this);

    if (al::isExistDitherAnimator(this)) {
        al::invalidateDitherAnim(this);
    }

    if (al::isExistCollisionParts(this)) {
        al::invalidateCollisionParts(this);
    }

    al::invalidateHitSensors(this);

    al::setClippingInfo(this, 999999999.0f, 0);
    al::setClippingNearDistance(this, 999999999.0f);
    al::validateClipping(this);

    al::offCollide(this);
    al::setScaleAll(this, 0.6f);

    al::showMaterial(this, "EyeMT");
    al::showMaterial(this, "BodyGoldMT");
    al::hideMaterial(this, "BodyRedMT");
    al::hideMaterial(this, "BodyBlueMT");

    mRotationAngle = 0.f;

    makeActorDead();
}

void ShineThiefPlayerBlock::initAfterPlacement(void) {
    al::LiveActor::initAfterPlacement();
}

bool ShineThiefPlayerBlock::receiveMsg(const al::SensorMsg* message, al::HitSensor* source, al::HitSensor* target) {
    return false;
}

void ShineThiefPlayerBlock::attackSensor(al::HitSensor* target, al::HitSensor* source) {
    return;
}

void ShineThiefPlayerBlock::updateMaterials() {
    auto* mode = GameModeManager::instance()->getMode<ShineThiefMode>();
    if (!mode)
        return;

    ShineThiefInfo* info = GameModeManager::instance()->getInfo<ShineThiefInfo>();
    if (!info)
        return;

    // Default: show only gold (for ground/non-team mode)
    al::showMaterial(this, "BodyGoldMT");
    al::hideMaterial(this, "BodyRedMT");
    al::hideMaterial(this, "BodyBlueMT");
    al::showMaterial(this, "EyeMT");

    // If in team mode and someone has the shine
    if (info->mIsTeamMode && info->mIsPlayerHolder) {
        // Player has it - check their team
        if (info->mPlayerTeam == ShineThiefTeam::TEAM_1) {
            // Team 1 (Blue) has it
            al::hideMaterial(this, "BodyGoldMT");
            al::hideMaterial(this, "BodyRedMT");
            al::showMaterial(this, "BodyBlueMT");
            al::showMaterial(this, "EyeMT");
        } else if (info->mPlayerTeam == ShineThiefTeam::TEAM_2) {
            // Team 2 (Red) has it
            al::hideMaterial(this, "BodyGoldMT");
            al::showMaterial(this, "BodyRedMT");
            al::hideMaterial(this, "BodyBlueMT");
            al::showMaterial(this, "EyeMT");
        }
    } else if (info->mIsTeamMode && !info->mIsPlayerHolder && info->mHolderPlayers.size() > 0) {
        // A puppet has it - check their team
        PuppetInfo* holder = info->mHolderPlayers.at(0);
        if (holder) {
            ShineThiefTeam holderTeam = (ShineThiefTeam)holder->shineThiefTeam;
            if (holderTeam == ShineThiefTeam::TEAM_1) {
                // Team 1 (Blue) has it
                al::hideMaterial(this, "BodyGoldMT");
                al::hideMaterial(this, "BodyRedMT");
                al::showMaterial(this, "BodyBlueMT");
                al::showMaterial(this, "EyeMT");
            } else if (holderTeam == ShineThiefTeam::TEAM_2) {
                // Team 2 (Red) has it
                al::hideMaterial(this, "BodyGoldMT");
                al::showMaterial(this, "BodyRedMT");
                al::hideMaterial(this, "BodyBlueMT");
                al::showMaterial(this, "EyeMT");
            }
        }
    }
}

void ShineThiefPlayerBlock::updateRotation() {
    auto* mode = GameModeManager::instance()->getMode<ShineThiefMode>();
    if (!mode)
        return;

    PlayerActorHakoniwa* player = mode->getPlayerActorHakoniwa();
    if (!player)
        return;

    sead::Vector3f rot = al::getRotate(this);

    if (player->mDimensionKeeper->mIs2D) {
        rot.y = 90.f;
        al::updatePoseRotate(this, rot);
    } else {
        mRotationAngle += 90.f * Time::deltaTime;
        if (mRotationAngle >= 360.f)
            mRotationAngle -= 360.f;

        rot.y = mRotationAngle;
        al::updatePoseRotate(this, rot);
    }
}

void ShineThiefPlayerBlock::control() {
    al::LiveActor::control();

    auto* mode = GameModeManager::instance()->getMode<ShineThiefMode>();
    if (!mode)
        return;

    PlayerActorHakoniwa* player = mode->getPlayerActorHakoniwa();
    if (!player)
        return;

    updateRotation();
    updateMaterials();
}

void ShineThiefPlayerBlock::appear() {
    al::LiveActor::appear();
    al::setNerve(this, &NrvShineThiefPlayerBlock.Appear);
    mIsLocked = true;
}

void ShineThiefPlayerBlock::end() {
    al::setNerve(this, &NrvShineThiefPlayerBlock.Disappear);
    mIsLocked = false;
}

void ShineThiefPlayerBlock::exeAppear() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "Appear");
        al::setScaleAll(this, 0.f);
    }

    updateRotation();
    updateMaterials();

    float targetScale = getBlockTargetScale();
    float s = al::lerpValue(al::getScaleX(this), targetScale, 0.2f);
    al::setScaleAll(this, s);

    mDitheringOffset = -420.f;
    al::setDitherAnimSphereRadius(this, 0.f);

    if (al::isNearZero(s - targetScale, 0.05f))
        al::setNerve(this, &NrvShineThiefPlayerBlock.Wait);
}

void ShineThiefPlayerBlock::exeWait() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "Wait");
    }
    al::setScaleAll(this, getBlockTargetScale());

    updateRotation();
    updateMaterials();

    mDitheringOffset = al::lerpValue(mDitheringOffset, -65.f, 0.02f);

    al::CameraPoser* curPoser = nullptr;
    al::CameraDirector* director = mSceneInfo->cameraDirector;

    if (director) {
        al::CameraPoseUpdater* updater = director->getPoseUpdater(0);
        if (updater && updater->mTicket)
            curPoser = updater->mTicket->mPoser;
    }

    if (curPoser) {
        float dist = al::calcDistance(this, curPoser->mPosition);
        al::setDitherAnimSphereRadius(this, dist + mDitheringOffset);
    }
}

void ShineThiefPlayerBlock::exeDisappear() {
    float scale = al::lerpValue(al::getScaleX(this), 0.f, 0.2f);
    al::setScaleAll(this, scale);

    if (al::isNearZero(scale, 0.05f))
        al::setNerve(this, &NrvShineThiefPlayerBlock.Dead);
}

void ShineThiefPlayerBlock::exeDead() {
    if (al::isFirstStep(this))
        kill();
}