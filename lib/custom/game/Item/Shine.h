#pragma once

#include <math/seadVector.h>
#include <prim/seadSafeString.h>

#include "Library/LiveActor/LiveActor.h"
#include "Util/IUseDimension.h"

struct ShineInfo;
class QuestInfo;

enum ShineType { Normal, Dot, Grand };

namespace al {
class RateParamV3f;
}

class Shine : public al::LiveActor,
              public IUseDimension {
public:
    Shine(const char*);

    void init(const al::ActorInitInfo&) override;
    al::LiveActor* getCurrentModel();
    bool tryExpandShadowAndClipping();
    void initAppearDemo(const al::ActorInitInfo&);
    void onAppear();
    void offAppear();
    void hideAllModel();
    void invalidateKillSensor();
    void initAfterPlacement() override;
    void getDirect();
    void updateHintTrans(const sead::Vector3f&) const;
    void appear() override;
    void makeActorAlive() override;
    void makeActorDead() override;
    void control() override;
    void updateModelActorPose();
    void attackSensor(al::HitSensor* self, al::HitSensor* other) override;
    bool receiveMsg(const al::SensorMsg* message, al::HitSensor* other, al::HitSensor* self) override;
    void showCurrentModel();
    void appearPopup();
    void addDemoActorWithModel();
    void get();
    void endClipped() override;
    void initAppearDemoFromHost(const al::ActorInitInfo&, const sead::Vector3f&);
    void initAppearDemoFromHost(const al::ActorInitInfo&);
    void initAppearDemoActorCamera(const al::ActorInitInfo&);
    void createShineEffectInsideObject(const al::ActorInitInfo&, const sead::Vector3f&, const char*);
    bool isGot() const;
    bool isEmptyShineForDemoGetGrand() const;
    void setShopShine();
    bool isEndAppear() const;
    bool isEndAppearGK() const;
    void onSwitchGet();
    s32 getColorFrame() const;
    void setHintPhotoShine(const al::ActorInitInfo&);
    bool appearCommon();
    bool tryChangeCoin();
    void tryAppearOrDemoAppear();

    void appearPopup(const sead::Vector3f&);
    void appearPopupDelay(s32);
    void appearPopupSlot(const sead::Vector3f&);
    void appearWarp(const sead::Vector3f&, const sead::Vector3f&);
    void appearStatic();
    void appearPopupWithoutDemo();
    void appearPopupGrandByBoss(s32);
    void appearPopupWithoutWarp();
    void appearAndJoinBossDemo(const char*, const sead::Quatf&, const sead::Vector3f&);

    void endBossDemo();
    void endBossDemoAndStartFall(f32);
    void appearWait();
    void appearWait(const sead::Vector3f&);
    void startHold();
    void startFall();
    void getDirectWithDemo();
    void addDemoModelActor();
    void setGrandShine();
    void exeWaitRequestDemo();
    void exeWaitKill();
    void exeDemoAppear();
    bool tryWaitCameraInterpole() const;
    bool tryStartAppearDemo();
    void calcCameraAt();

    void exeDemoMove();
    void updateIgnoreFrame();
    void exeDemoWait();
    void exeDemoGet();
    void exeDemoGetMain();
    void exeDemoGetGrand();
    void exeBossDemo();
    void exeBossDemoAfterFall();
    void exeBossDemoAfterLanding();
    void exeBossDemoFall();
    void exeBossDemoFallSlowdown();
    void exeBossDemoRise();
    void exeBossDemoRiseDamp();
    void exeAppearSlot();
    void exeAppearSlotDown();
    void exeAppear();
    void exeAppearWait();
    void exeAppearDown();
    void exeAppearStatic();
    void exeAppearEnd();
    void exeAppearWaitCameraInterpole();
    void exeWait();
    void exeGot();
    void exeHold();
    void exeFall();
    void exeDelay();
    void exeHide();
    void exeReaction();
    void exeCoin();

    void updateModelActorResetPosition();
    ActorDimensionKeeper* getActorDimensionKeeper() const override;

    bool isMainShine() const { return mIsMainShine; }

public:
    void* qword110;
    int dword118;
    bool mIsGotShine;
    ShineInfo* curShineInfo; // 0x120
    unsigned char padding_188[0x188 - 0x128];
    al::RateParamV3f* mRateParam;
    void* qword190;
    void* qword198;
    ShineType mModelType;
    void* qword1A8;
    bool byte1B0;
    void* qword1B8;
    int dword1C0;
    int dword1C4;
    sead::FixedSafeString<0x80> mShineLabel;
    void* qword260;
    int dword268;
    bool byte26C;
    void* qword270;
    QuestInfo* shineQuestInfo;              // 0x278
    void* unkPtr1;                          // 0x280
    ActorDimensionKeeper* mDimensionKeeper; // 0x288
    int mShineIdx;                          // 0x290
    bool mIsMainShine;
    void* qword298;
    void* qword2A0;
    void* qword2A8;
    void* qword2B0;
    void* qword2B8;
    int dword2C0;
    __attribute__((packed)) void* qword2C4;
    int dword2CC;
    int dword2D0;
    bool mIsAddHeight;
    int dword2D8;
    al::LiveActor* mModelEmpty;
    al::LiveActor* mModelShine;
    int dword2F0;
    u16 word2F4;
    int dword2F8;
    bool mIsNoRotate;
    void* qword300;
    bool mIsUseDemoCam;
    struct WaterSurfaceShadow* mWaterShadow;
    void* qword318;
    int dword320;
    int dword324;
    bool byte328;
    void* qword330;
    bool mIsCheckGroundHeightMoon;
    bool mIsHintPhoto;
    void* qword340;
    bool byte348;
    void* qword350;
    bool mIsUseAppearDemoForce;
    int dword35C;
    int dword360;
    int dword364;
    int dword368;
    bool mIsPowerStar;
    bool mIsAppearDemoHeightHigh;
    void* qword370;
    u16 word378;
    int dword37C;
};

static_assert(sizeof(Shine) == 0x380);

namespace ShineFunction {
const char* getMovePointLinkName();
}
