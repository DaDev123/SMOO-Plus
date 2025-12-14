#include "puppets/PuppetHolder.hpp"
#include <math.h>
#include "actors/PuppetActor.h"
#include "al/util.hpp"
#include "al/util/LiveActorUtil.h"
#include "container/seadPtrArray.h"
#include "heap/seadHeap.h"
#include "heap/seadHeapMgr.h"
#include "logger.hpp"

PuppetHolder::PuppetHolder(int size) {
    if(!mPuppetArr.tryAllocBuffer(size, nullptr)) {
        Logger::log("[PuppetHolder] ERROR: Buffer Alloc Failed on Puppet Holder!\n");
    } else {
        Logger::log("[PuppetHolder] Successfully allocated buffer for %d puppets\n", size);
    }
}

/**
 * @brief resizes puppet ptr array by creating a new ptr array and storing previous ptrs in it, before freeing the previous array
 * 
 * @param size the size of the new ptr array
 * @return returns true if resizing was successful
 */
bool PuppetHolder::resizeHolder(int size) {

    if (mPuppetArr.capacity() == size) {
        Logger::log("[PuppetHolder] No resize needed, already at capacity %d\n", size);
        return true;  // no need to resize if we're already at the same capacity
    }

    sead::Heap *seqHeap = sead::HeapMgr::instance()->findHeapByName("SequenceHeap", 0);

    if (!mPuppetArr.isBufferReady()) {
        bool result = mPuppetArr.tryAllocBuffer(size, seqHeap);
        Logger::log("[PuppetHolder] Initial buffer allocation %s for size %d\n", 
                    result ? "succeeded" : "FAILED", size);
        return result;
    }
        
    sead::PtrArray<PuppetActor> newPuppets = sead::PtrArray<PuppetActor>();

    if (newPuppets.tryAllocBuffer(size, seqHeap)) {
        
        int curPupCount = mPuppetArr.size();
        int copyCount = (curPupCount > size) ? size : curPupCount;

        Logger::log("[PuppetHolder] Resizing from %d to %d, copying %d puppets\n", 
                    mPuppetArr.capacity(), size, copyCount);

        for (int i = 0; i < copyCount; i++) {
            newPuppets.pushBack(mPuppetArr[i]);
        }

        mPuppetArr.freeBuffer();
        mPuppetArr = newPuppets;

        Logger::log("[PuppetHolder] Resize successful\n");
        return true;
    } else {
        Logger::log("[PuppetHolder] ERROR: Failed to allocate new buffer for resize\n");
        return false;
    }
}

bool PuppetHolder::tryRegisterPuppet(PuppetActor *puppet) {
    if(!mPuppetArr.isFull()) {
        mPuppetArr.pushBack(puppet);
        Logger::log("[PuppetHolder] Registered puppet %d/%d\n", 
                    mPuppetArr.size(), mPuppetArr.capacity());
        return true;
    }else {
        Logger::log("[PuppetHolder] ERROR: Cannot register puppet, holder is full (%d/%d)\n",
                    mPuppetArr.size(), mPuppetArr.capacity());
        return false;
    }
}

bool PuppetHolder::tryRegisterDebugPuppet(PuppetActor *puppet) {
    mDebugPuppet = puppet;
    Logger::log("[PuppetHolder] Debug puppet registered\n");
    return true;
}

PuppetActor *PuppetHolder::getDebugPuppet() {
    if(mDebugPuppet) {
        return mDebugPuppet;
    }
    return nullptr;
}

void PuppetHolder::update() {

    for (size_t i = 0; i < mPuppetArr.size(); i++)
    {
        PuppetActor *curPuppet = mPuppetArr[i];
        
        if (!curPuppet) {
            Logger::log("[PuppetHolder] WARNING: Null puppet at index %zu\n", i);
            continue;
        }

        PuppetInfo *curInfo = curPuppet->getInfo();
        
        if (!curInfo) {
            Logger::log("[PuppetHolder] WARNING: Null info for puppet at index %zu\n", i);
            continue;
        }

        bool wasInStage = curInfo->isInSameStage;
        curInfo->isInSameStage = checkInfoIsInStage(curInfo);

        // Log stage transitions for debugging
        if (wasInStage != curInfo->isInSameStage && curPuppet->mIsDebug) {
            Logger::log("[PuppetHolder] Puppet '%s' stage status changed: %s -> %s\n",
                       curInfo->puppetName,
                       wasInStage ? "in stage" : "out of stage",
                       curInfo->isInSameStage ? "in stage" : "out of stage");
        }

        if(curInfo->isInSameStage && al::isDead(curPuppet)) {
            curPuppet->makeActorAlive();
            
            if (curPuppet->mIsDebug) {
                Logger::log("[PuppetHolder] Puppet '%s' made alive (entered stage)\n", 
                           curInfo->puppetName);
            }

            //curPuppet->emitJoinEffect();  //Poof Particles
        }else if(!curInfo->isInSameStage && !al::isDead(curPuppet)) {
            curPuppet->makeActorDead();
            
            if (curPuppet->mIsDebug) {
                Logger::log("[PuppetHolder] Puppet '%s' made dead (left stage)\n", 
                           curInfo->puppetName);
            }
            
            //curPuppet->emitJoinEffect(); //Poof Particles
        }
    }
}

bool PuppetHolder::checkInfoIsInStage(PuppetInfo *info) {
    if (!info) {
        return false;
    }

    if (info->isConnected) {
        // For scenarios 0-14 (main game scenarios), only check stage name
        if (info->scenarioNo < 15) {
            return al::isEqualString(mStageName.cstr(), info->stageName);
        } else {
            // For scenario 15+ (likely bonus/special stages), check both stage name and scenario
            return al::isEqualString(mStageName.cstr(), info->stageName) && 
                   info->scenarioNo == mScenarioNo;
        }
    }
    
    return false;
}

void PuppetHolder::setStageInfo(const char *stageName, u8 scenarioNo) {
    if (stageName) {
        mStageName = stageName;
        mScenarioNo = scenarioNo;
        Logger::log("[PuppetHolder] Stage info updated: %s, Scenario %d\n", 
                   stageName, scenarioNo);
    } else {
        Logger::log("[PuppetHolder] WARNING: Attempted to set null stage name\n");
    }
}