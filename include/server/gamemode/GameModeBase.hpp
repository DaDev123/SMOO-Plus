#pragma once

#include <cmath>
#include <math.h>
#include "al/sensor/HitSensor.h"
#include "al/sensor/SensorMsg.h"
#include "puppets/PuppetHolder.hpp"
#include "al/actor/ActorInitInfo.h"
#include "al/actor/IUseName.h"
#include "al/scene/Scene.h"
#include "al/scene/SceneObjHolder.h"
#include "game/GameData/GameDataHolder.h"
#include "game/StageScene/StageScene.h"
#include "layouts/HideAndSeekIcon.h"
#include "prim/seadSafeString.h"
#include "server/gamemode/GameModeConfigMenu.hpp"

// enum for defining game mode types
enum GameMode : s8 {
    NONE = -1,
    HIDEANDSEEK,
    SARDINE,
    FREEZETAG
};

// struct containing info about the games state for use in gamemodes
struct GameModeInitInfo {
    GameModeInitInfo(al::ActorInitInfo* info, al::Scene *scene){
        mLayoutInitInfo = info->mLayoutInitInfo;
        mActorInitInfo = info;
        mPlayerHolder = info->mActorSceneInfo.mPlayerHolder;
        mSceneObjHolder = info->mActorSceneInfo.mSceneObjHolder;
        mScene = scene;
        
    };

    void initServerInfo(GameMode mode, PuppetHolder *pupHolder) {
        mMode = mode;
        mPuppetHolder = pupHolder;
    }

    al::LayoutInitInfo* mLayoutInitInfo;
    al::ActorInitInfo *mActorInitInfo;
    al::PlayerHolder* mPlayerHolder;
    al::SceneObjHolder *mSceneObjHolder;
    al::Scene* mScene;
    GameMode mMode = GameMode::NONE;
    PuppetHolder *mPuppetHolder;
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

    virtual void init(GameModeInitInfo const &info);

    virtual void begin();

    virtual void update(){};
    virtual void end();

    virtual void pause() { mIsActive = false; };
    virtual void unpause() { mIsActive = true; };

    virtual bool receiveMsg(const al::SensorMsg *msg, al::HitSensor *source, al::HitSensor *target) { return false; };

    virtual bool attackSensor(al::HitSensor* source, al::HitSensor* target) { return false; };

    virtual void processPacket(Packet* packet){};

    virtual Packet* createPacket() { return nullptr; }

    bool mIsUsePuppetSensor;
    bool mIsUseCapSensor;

protected:
    sead::FixedSafeString<0x10> mName;
    al::SceneObjHolder *mSceneObjHolder = nullptr;
    GameMode mMode = GameMode::NONE;
    StageScene* mCurScene = nullptr;
    PuppetHolder *mPuppetHolder = nullptr;
    bool mIsActive = false;
    bool mIsFirstFrame = true;
};