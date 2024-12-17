#pragma once

#include "al/actor/IUseName.h"
#include "al/scene/SceneObjHolder.h"

#include "game/HakoniwaSequence/HakoniwaSequence.h"
#include "game/StageScene/StageScene.h"

#include "puppets/PuppetHolder.hpp"

#include "sead/devenv/seadDebugFontMgrNvn.h"
#include "sead/gfx/seadTextWriter.h"
#include "sead/prim/seadSafeString.h"

#include "server/gamemode/GameMode.hpp"
#include "server/gamemode/GameModeInitInfo.hpp"

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
    virtual bool ignoreComboBtn() const { return false; }
    virtual bool pauseTimeWhenPaused() const { return false; }

    virtual void init(GameModeInitInfo const& info);

    virtual void begin();
    virtual void update() {};
    virtual void end();

    virtual void pause() { mIsActive = false; };
    virtual void unpause() { mIsActive = true; };

    virtual bool showNameTag(PuppetInfo* other) { return false; }
    virtual bool showNameTagEverywhere(PuppetActor* other) { return false; }

    virtual void debugMenuControls(sead::TextWriter* gTextWriter) {}
    virtual void debugMenuPlayer(sead::TextWriter* gTextWriter, PuppetInfo* other = nullptr) {}

    virtual void processPacket(Packet* packet) {};

    virtual Packet* createPacket() { return nullptr; }

    virtual void onHakoniwaSequenceFirstStep(HakoniwaSequence* sequence) {}
    virtual void onBorderPullBackFirstStep(al::LiveActor* actor) {}

    virtual bool hasCustomCamera() const { return false; }
    virtual void createCustomCameraTicket(al::CameraDirector* director) {}

    virtual bool hasMarioCollision() { return true;  }
    virtual bool hasMarioBounce()    { return true;  }
    virtual bool hasCappyCollision() { return false; }
    virtual bool hasCappyBounce()    { return false; }

protected:
    sead::FixedSafeString<0x10> mName;
    al::SceneObjHolder*         mSceneObjHolder = nullptr;
    GameMode                    mMode           = GameMode::NONE;
    StageScene*                 mCurScene       = nullptr;
    PuppetHolder*               mPuppetHolder   = nullptr;
    bool                        mIsActive       = false;
    bool                        mIsFirstFrame   = true;
};
