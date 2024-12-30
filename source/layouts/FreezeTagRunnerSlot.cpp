#include "layouts/FreezeTagRunnerSlot.h"
#include "al/string/StringTmp.h"
#include "al/util.hpp"
#include "al/util/MathUtil.h"
#include "logger.hpp"
#include "main.hpp"
#include "math/seadVector.h"
#include "prim/seadSafeString.h"
#include "puppets/PuppetInfo.h"
#include "rs/util.hpp"
#include "server/Client.hpp"
#include "server/freeze/FreezeTagMode.hpp"
#include <cstdio>
#include <cstring>

FreezeTagRunnerSlot::FreezeTagRunnerSlot(const char* name, const al::LayoutInitInfo& initInfo)
    : al::LayoutActor(name)
{
    al::initLayoutActor(this, initInfo, "FreezeTagRunnerSlot", 0);
    mInfo = GameModeManager::instance()->getInfo<FreezeTagInfo>();

    initNerve(&nrvFreezeTagRunnerSlotEnd, 0);
    kill();
}

void FreezeTagRunnerSlot::init(int index)
{
    // Place slot based on index and hide
    al::setPaneLocalTrans(this, "RunnerSlot", { -580.f, 270.f - (index * 55.f), 0.f });
    al::hidePane(this, "RunnerSlot");

    // Set temporary name string
    al::setPaneString(this, "TxtRunnerName", u"MaxLengthNameAaa", 0);

    mRunnerIndex = index;
    return;
}

void FreezeTagRunnerSlot::appear()
{
    al::startAction(this, "Appear", 0);
    al::setNerve(this, &nrvFreezeTagRunnerSlotAppear);
    al::LayoutActor::appear();
}

bool FreezeTagRunnerSlot::tryEnd()
{
    if (!al::isNerve(this, &nrvFreezeTagRunnerSlotEnd)) {
        al::setNerve(this, &nrvFreezeTagRunnerSlotEnd);
        return true;
    }
    return false;
}

bool FreezeTagRunnerSlot::tryStart()
{
    if (!al::isNerve(this, &nrvFreezeTagRunnerSlotWait) && !al::isNerve(this, &nrvFreezeTagRunnerSlotAppear)) {
        appear();
        return true;
    }

    return false;
}

void FreezeTagRunnerSlot::exeAppear()
{
    if (al::isActionEnd(this, 0)) {
        al::setNerve(this, &nrvFreezeTagRunnerSlotWait);
    }
}

void FreezeTagRunnerSlot::exeWait()
{
    if (al::isFirstStep(this)) {
        al::startAction(this, "Wait", 0);
    }

    mIsPlayer = mRunnerIndex == 0 && mInfo->mIsPlayerRunner;

    // Show/hide icon if player doesn't exist in this slot
    if (mRunnerIndex >= mInfo->mRunnerPlayers.size() + mInfo->mIsPlayerRunner) {
        if (mIsVisible)
            hideSlot();
    } else if (!mIsVisible)
        showSlot();

    if (!mIsVisible) // If icon isn't visible, end wait processing here
        return;

    mFreezeIconSpin += 1.2f;
    if (mFreezeIconSpin > 360.f + (mRunnerIndex * 7.5f))
        mFreezeIconSpin -= 360.f;

    setFreezeAngle();

    // Update name info in this slot
    if (mIsPlayer) {
        setSlotName(Client::instance()->getClientName());
        setSlotScore(mInfo->mPlayerTagScore.mScore);
    } else {
        if (mRunnerIndex >= mInfo->mRunnerPlayers.size() + mInfo->mIsPlayerRunner)
            return;

        setSlotName(mInfo->mRunnerPlayers.at(mRunnerIndex - mInfo->mIsPlayerRunner)->puppetName);
        setSlotScore(mInfo->mRunnerPlayers.at(mRunnerIndex - mInfo->mIsPlayerRunner)->freezeTagScore);
    }
}

void FreezeTagRunnerSlot::exeEnd()
{

    if (al::isFirstStep(this)) {
        al::startAction(this, "End", 0);
    }

    if (al::isActionEnd(this, 0)) {
        kill();
    }
}

void FreezeTagRunnerSlot::showSlot()
{
    mIsVisible = true;
    al::showPane(this, "RunnerSlot");
}

void FreezeTagRunnerSlot::hideSlot()
{
    mIsVisible = false;
    al::hidePane(this, "RunnerSlot");
}

void FreezeTagRunnerSlot::setFreezeAngle()
{
    al::setPaneLocalRotate(this, "PicRunnerFreeze", { 0.f, 0.f, mFreezeIconSpin + (mRunnerIndex * 7.5f) });

    if (mIsPlayer) {
        float targetSize = mInfo->mIsPlayerFreeze ? 1.f : 0.f;
        mInfo->mFreezeIconSize = al::lerpValue(mInfo->mFreezeIconSize, targetSize, 0.05f);
        al::setPaneLocalScale(this, "PicRunnerFreeze", { mInfo->mFreezeIconSize, mInfo->mFreezeIconSize });
    } else {
        if (mRunnerIndex >= mInfo->mRunnerPlayers.size() + mInfo->mIsPlayerRunner)
            return;

        PuppetInfo* curInfo = mInfo->mRunnerPlayers.at(mRunnerIndex - mInfo->mIsPlayerRunner);

        float targetSize = curInfo->isFreezeTagFreeze ? 1.f : 0.f;
        curInfo->freezeIconSize = al::lerpValue(curInfo->freezeIconSize, targetSize, 0.05f);
        al::setPaneLocalScale(this, "PicRunnerFreeze", { curInfo->freezeIconSize, curInfo->freezeIconSize });
    }
}

namespace {
NERVE_IMPL(FreezeTagRunnerSlot, Appear)
NERVE_IMPL(FreezeTagRunnerSlot, Wait)
NERVE_IMPL(FreezeTagRunnerSlot, End)
}