#pragma once

#include "al/Library/HostIO/IUseName.h"
#include "al/Library/LiveActor/ActorInitInfo.h"
#include "al/Library/Scene/Scene.h"
#include "al/Library/Scene/SceneObjHolder.h"

#include "game/Scene/StageScene.h"

#include <math.h>

#include "Library/Layout/LayoutInitInfo.h"
#include "prim/seadSafeString.h"
#include "puppets/PuppetHolder.hpp"
#include "server/gamemode/GameMode.h"

// struct containing info about the games state for use in gamemodes
struct GameModeInitInfo {
    GameModeInitInfo(al::ActorInitInfo* info, al::Scene* scene) {
        mLayoutInitInfo = const_cast<al::LayoutInitInfo*>(info->layoutInitInfo);
        mActorInitInfo = info;
        mPlayerHolder = info->actorSceneInfo.playerHolder;
        mSceneObjHolder = info->actorSceneInfo.sceneObjHolder;
        mScene = scene;
    };

    void initServerInfo(GameMode mode, PuppetHolder* pupHolder) {
        mMode = mode;
        mPuppetHolder = pupHolder;
    }

    al::LayoutInitInfo* mLayoutInitInfo;
    al::ActorInitInfo* mActorInitInfo;
    al::PlayerHolder* mPlayerHolder;
    al::SceneObjHolder* mSceneObjHolder;
    al::Scene* mScene;
    GameMode mMode = GameMode::NONE;
    PuppetHolder* mPuppetHolder;
};

// base class for all gamemodes, must inherit from this to have a functional gamemode
class GameModeBase : public al::IUseName, public al::IUseSceneObjHolder {
public:
    GameModeBase(const char* name) { mName = name; }
    virtual ~GameModeBase() = default;
    const char* getName() const override { return mName.cstr(); }
    al::SceneObjHolder* getSceneObjHolder() const override { return mSceneObjHolder; }

    virtual GameMode getMode() { return mMode; }

    virtual bool isModeActive() const { return mIsActive; }
    virtual bool isUseNormalUI() const { return true; }

    virtual void init(GameModeInitInfo const& info);

    virtual void begin();
    virtual void update() { return; };
    virtual void end();

    virtual void pause() { mIsActive = false; };
    virtual void unpause() { mIsActive = true; };

protected:
    sead::FixedSafeString<0x10> mName;
    al::SceneObjHolder* mSceneObjHolder = nullptr;
    GameMode mMode = GameMode::NONE;
    StageScene* mCurScene = nullptr;
    PuppetHolder* mPuppetHolder = nullptr;
    bool mIsActive = false;
    bool mIsFirstFrame = true;
};