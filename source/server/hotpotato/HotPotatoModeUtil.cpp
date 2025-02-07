#include "server/hotpotato/HotPotatoMode.hpp"

bool HotPotatoMode::isPlayerLastSurvivor(PuppetInfo* changingPuppet)
{
    if(!mInfo->mIsPlayerRunner)
        return false; // If player is on the chaser team, just return false instantly
    
    if(mInfo->mRunnerPlayers.size() == 0)
        return false; // If there's no other player on the runner team, last survivor stuff is disabled
    
    for(int i = 0; i < mInfo->mRunnerPlayers.size(); i++) {
        PuppetInfo* inf = mInfo->mRunnerPlayers.at(i);
        if(changingPuppet == inf)
            continue; // If the puppet getting updated is the one currently being checked, skip this one

        if(!inf->isHotPotatoFreeze)
            return false; // Found another non-frozen player, not last survivor
    }

    return true; //Last survivor check passed!
}

bool HotPotatoMode::isAllRunnerFrozen(PuppetInfo* changingPuppet)
{
    if(mInfo->mRunnerPlayers.size() < 2 - mInfo->mIsPlayerRunner)
        return false; // Verify there is at least two runners (including yourself), otherwise disable this functionality
    
    if(mInfo->mIsPlayerRunner && !mInfo->mIsPlayerFreeze)
        return false; // If you are a runner but aren't frozen then skip
    
    for(int i = 0; i < mInfo->mRunnerPlayers.size(); i++) {
        PuppetInfo* inf = mInfo->mRunnerPlayers.at(i);
        if(changingPuppet == inf)
            continue; // If the puppet getting updated is the one currently being checked, skip this one

        if(!inf->isHotPotatoFreeze)
            return false; // Found a non-frozen player on the runner team, cancel
    }

    return true; // All runners are frozen!
}

PlayerActorHakoniwa* HotPotatoMode::getPlayerActorHakoniwa()
{
    PlayerActorBase* playerBase = rs::getPlayerActor(mCurScene);
    bool isYukimaru = !playerBase->getPlayerInfo();

    if(isYukimaru)
        return nullptr;
    
    return (PlayerActorHakoniwa*)playerBase;
}