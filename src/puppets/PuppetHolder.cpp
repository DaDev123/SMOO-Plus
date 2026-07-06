#include "puppets/PuppetHolder.hpp"

#include "hk/diag/diag.h"

#include "sead/heap/seadHakkunHeap.h"

#include "al/Library/LiveActor/ActorFlagFunction.h"

#include <math.h>

#include "actors/PuppetActor.h"
#include "container/seadPtrArray.h"
#include "heap/seadHeap.h"

PuppetHolder::PuppetHolder(int size) {
    if (!mPuppetArr.tryAllocBuffer(size, sead::HakkunHeap::sInstance)) {
        hk::diag::logLine("[PuppetHolder] ERROR: Buffer Alloc Failed on Puppet Holder!");
    } else {
        hk::diag::logLine("[PuppetHolder] Successfully allocated buffer for %d puppets", size);
    }
}

/**
 * @brief resizes puppet ptr array by creating a new ptr array and storing previous ptrs in it,
 * before freeing the previous array
 *
 * @param size the size of the new ptr array
 * @return returns true if resizing was successful
 */
bool PuppetHolder::resizeHolder(int size) {
    if (mPuppetArr.capacity() == size) {
        hk::diag::logLine("[PuppetHolder] No resize needed, already at capacity %d", size);
        return true;  // no need to resize if we're already at the same capacity
    }

    sead::Heap* hkHeap = sead::HakkunHeap::sInstance;

    if (!mPuppetArr.isBufferReady()) {
        bool result = mPuppetArr.tryAllocBuffer(size, hkHeap);
        hk::diag::logLine("[PuppetHolder] Initial buffer allocation %s for size %d",
                          result ? "succeeded" : "FAILED", size);
        return result;
    }

    sead::PtrArray<PuppetActor> newPuppets = sead::PtrArray<PuppetActor>();

    if (newPuppets.tryAllocBuffer(size, hkHeap)) {
        int curPupCount = mPuppetArr.size();
        int copyCount = (curPupCount > size) ? size : curPupCount;

        hk::diag::logLine("[PuppetHolder] Resizing from %d to %d, copying %d puppets", mPuppetArr.capacity(),
                          size, copyCount);

        for (int i = 0; i < copyCount; i++) {
            newPuppets.pushBack(mPuppetArr[i]);
        }

        mPuppetArr.freeBuffer();
        mPuppetArr = newPuppets;

        hk::diag::logLine("[PuppetHolder] Resize successful");
        return true;
    } else {
        hk::diag::logLine("[PuppetHolder] ERROR: Failed to allocate new buffer for resize");
        return false;
    }
}

bool PuppetHolder::tryRegisterPuppet(PuppetActor* puppet) {
    if (!mPuppetArr.isFull()) {
        mPuppetArr.pushBack(puppet);
        hk::diag::logLine("[PuppetHolder] Registered puppet %d/%d", mPuppetArr.size(), mPuppetArr.capacity());
        return true;
    } else {
        hk::diag::logLine("[PuppetHolder] ERROR: Cannot register puppet, holder is full (%d/%d)",
                          mPuppetArr.size(), mPuppetArr.capacity());
        return false;
    }
}

bool PuppetHolder::tryRegisterDebugPuppet(PuppetActor* puppet) {
    mDebugPuppet = puppet;
    // hk::diag::logLine("[PuppetHolder] Debug puppet registered");
    return true;
}

PuppetActor* PuppetHolder::getDebugPuppet() {
    if (mDebugPuppet) {
        return mDebugPuppet;
    }
    return nullptr;
}

void PuppetHolder::update() {
    for (size_t i = 0; i < mPuppetArr.size(); i++) {
        PuppetActor* curPuppet = mPuppetArr[i];

        if (!curPuppet) {
            // hk::diag::logLine("[PuppetHolder] WARNING: Null puppet at index %zu", i);
            continue;
        }

        PuppetInfo* curInfo = curPuppet->getInfo();

        if (!curInfo) {
            // hk::diag::logLine("[PuppetHolder] WARNING: Null info for puppet at index %zu", i);
            continue;
        }

        bool wasInStage = curInfo->isInSameStage;
        curInfo->isInSameStage = checkInfoIsInStage(curInfo);

        // Log stage transitions for debugging
        if (wasInStage != curInfo->isInSameStage && curPuppet->mIsDebug) {
            // hk::diag::logLine("[PuppetHolder] Puppet '%s' stage status changed: %s -> %s",
            // curInfo->puppetName, wasInStage ? "in stage" : "out of stage", curInfo->isInSameStage ? "in
            // stage" : "out of stage");
        }

        if (curInfo->isInSameStage && al::isDead(curPuppet)) {
            curPuppet->makeActorAlive();

            if (curPuppet->mIsDebug) {
                // hk::diag::logLine("[PuppetHolder] Puppet '%s' made alive (entered stage)",
                // curInfo->puppetName);
            }

            // curPuppet->emitJoinEffect();  //Poof Particles
        } else if (!curInfo->isInSameStage && !al::isDead(curPuppet)) {
            curPuppet->makeActorDead();

            if (curPuppet->mIsDebug) {
                // hk::diag::logLine("[PuppetHolder] Puppet '%s' made dead (left stage)",
                // curInfo->puppetName);
            }

            // curPuppet->emitJoinEffect(); //Poof Particles
        }
    }
}

bool PuppetHolder::checkInfoIsInStage(PuppetInfo* info) {
    if (!info) {
        return false;
    }

    if (info->isConnected) {
        // For scenarios 0-14 (main game scenarios), only check stage name
        if (info->scenarioNo < 15) {
            return al::isEqualString(mStageName.cstr(), info->stageName);
        } else {
            // For scenario 15+ (likely bonus/special stages), check both stage name and scenario
            return al::isEqualString(mStageName.cstr(), info->stageName) && info->scenarioNo == mScenarioNo;
        }
    }

    return false;
}

void PuppetHolder::setStageInfo(const char* stageName, u8 scenarioNo) {
    if (stageName) {
        mStageName = stageName;
        mScenarioNo = scenarioNo;
        // hk::diag::logLine("[PuppetHolder] Stage info updated: %s, Scenario %d", stageName, scenarioNo);
    } else {
        // hk::diag::logLine("[PuppetHolder] WARNING: Attempted to set null stage name");
    }
}