#pragma once

#include "game/Sequence/HakoniwaSequence.h"
#include "al/Library/Scene/Scene.h"
#include "game/Scene/StageScene.h"
#include "game/Player/PlayerActorBase.h"
#include "game/Player/PlayerActorHakoniwa.h"

namespace helpers {

    bool isInScene();
    bool isInScene(al::Scene* scene);
    bool isInStageScene();
    bool isInStageScene(al::Scene* scene);
    bool tryReloadStage();
    
    // Safe getters
    al::Sequence* tryGetSequence();
    HakoniwaSequence* tryGetHakoniwaSequence();
    
    al::Scene* tryGetScene();
    al::Scene* tryGetScene(al::Sequence* curSequence);
    al::Scene* tryGetScene(HakoniwaSequence* curSequence);
    StageScene* tryGetStageScene();
    StageScene* tryGetStageScene(HakoniwaSequence* curSequence);
    
    GameDataHolder* tryGetGameDataHolder();
    GameDataHolder* tryGetGameDataHolder(HakoniwaSequence* curSequence);
    GameDataHolder* tryGetGameDataHolder(StageScene* scene);
    GameDataHolderAccessor* tryGetGameDataHolderAccess();
    GameDataHolderAccessor* tryGetGameDataHolderAccess(HakoniwaSequence* curSequence);
    GameDataHolderAccessor* tryGetGameDataHolderAccess(StageScene* scene);
    
    PlayerActorBase* tryGetPlayerActor();
    PlayerActorBase* tryGetPlayerActor(HakoniwaSequence* curSequence);
    PlayerActorBase* tryGetPlayerActor(al::Scene* scene);
    PlayerActorHakoniwa* tryGetPlayerActorHakoniwa();
    PlayerActorHakoniwa* tryGetPlayerActorHakoniwa(HakoniwaSequence* curSequence);
    PlayerActorHakoniwa* tryGetPlayerActorHakoniwa(al::Scene* scene);

    bool isGetShineState(StageScene* stageScene);

    char* demangle(const char* mangled_name);

} // namespace helpers