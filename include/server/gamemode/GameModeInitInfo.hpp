#pragma once

#include "puppets/PuppetHolder.hpp"
#include "al/actor/ActorInitInfo.h"
#include "al/scene/Scene.h"
#include "al/scene/SceneObjHolder.h"
#include "server/gamemode/GameMode.hpp"

// struct containing info about the games state for use in gamemodes
struct GameModeInitInfo {
    GameModeInitInfo(al::ActorInitInfo* info, al::Scene* scene) {
        mLayoutInitInfo = info->mLayoutInitInfo;
        mActorInitInfo  = info;
        mPlayerHolder   = info->mActorSceneInfo.mPlayerHolder;
        mSceneObjHolder = info->mActorSceneInfo.mSceneObjHolder;
        mScene          = scene;
    };

    void initServerInfo(GameMode mode, PuppetHolder* pupHolder) {
        mMode         = mode;
        mPuppetHolder = pupHolder;
    }

    al::LayoutInitInfo* mLayoutInitInfo;
    al::ActorInitInfo*  mActorInitInfo;
    al::PlayerHolder*   mPlayerHolder;
    al::SceneObjHolder* mSceneObjHolder;
    al::Scene*          mScene;
    GameMode            mMode = GameMode::NONE;
    PuppetHolder*       mPuppetHolder;
};
