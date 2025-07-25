#pragma once

#include "al/LiveActor/LiveActor.h"

namespace rs {
    bool isPlayerHack(al::LiveActor const*);
    bool isPlayerHackType(al::LiveActor const*,int);
    bool isPlayerHackRigidBody(al::LiveActor const*);
    bool isPlayerHackJugemFishing(al::LiveActor const*);
    bool isPlayerHackKuriboAny(al::LiveActor const*);
    bool isPlayerHackKuribo(al::LiveActor const*);
    bool isPlayerHackKuriboWing(al::LiveActor const*);
    bool isPlayerHackStatueMario(al::LiveActor const*);
    bool isPlayerHackEnemy(al::LiveActor const*);
    bool isPlayerHackTrilemmaRock(al::LiveActor const*);
    bool isPlayerHackTrilemmaPaper(al::LiveActor const*);
    bool isPlayerHackTrilemmaScissors(al::LiveActor const*);
    bool isPlayerHackElectricWire(al::LiveActor const*);
    bool isPlayerHackTRex(al::LiveActor const*);
    bool isPlayerHackFukankun(al::LiveActor const*);
    bool isPlayerHackHosui(al::LiveActor const*);
    bool isPlayerHackYoshi(al::LiveActor const*);
    bool isPlayerHackYukimaru(al::LiveActor const*);
    bool isPlayerHackHammerBros(al::LiveActor const*);
    bool isPlayerHackBazookaElectric(al::LiveActor const*);
    bool isPlayerHackBubble(al::LiveActor const*);
    bool isPlayerHackTank(al::LiveActor const*);
    bool isPlayerHackTsukkun(al::LiveActor const*);
    bool isPlayerHackPukupuku(al::LiveActor const*);
    bool isPlayerHackPukupukuAll(al::LiveActor const*);
    bool isPlayerHackRadiconNpc(al::LiveActor const*);
    bool isPlayerHackSenobi(al::LiveActor const*);
    bool isPlayerHackKakku(al::LiveActor const*);
    bool isPlayerHackGroupTalkScare(al::LiveActor const*);
    bool isPlayerHackGroupUseCameraStick(al::LiveActor const*);
    bool isPlayerHackNoSeparateCameraInput(al::LiveActor const*);

    bool isPlayerNoInput(al::LiveActor const *);

    void calcGuidePos(sead::Vector3f*, al::LiveActor const*);

    bool isActiveDemoPlayerPuppetable(al::LiveActor const*);
}