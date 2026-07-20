#pragma once

#include "nn/account.h"

#include "sead/math/seadQuat.h"
#include "sead/prim/seadSafeString.h"

#include "algorithms/PlayerAnims.h"
#include "packets/Packet.h"
struct PuppetInfo {
    // General Puppet Info
    sead::FixedSafeString<0x10>
        puppetName;  // max user account name size is 10 chars, so this could go down to 0xB
    bool isConnected = false;
    nn::account::Uid playerID;
    // Puppet Translation Info
    sead::Vector3f playerPos = sead::Vector3f(0.f, 0.f, 0.f);
    sead::Quatf playerRot = sead::Quatf(0.f, 0.f, 0.f, 0.f);
    // Puppet Stage Info
    u8 scenarioNo = -1;
    sead::FixedSafeString<0x40> stageName;
    bool isInSameStage = false;
    // Puppet Costume Info
    sead::FixedSafeString<0x20> costumeBody;
    sead::FixedSafeString<0x20> costumeHead;
    // Puppet Capture Info
    sead::FixedSafeString<0x40> curHack;
    bool isCaptured = false;
    bool isStartCapture = false;
    // Puppet Model Info
    PlayerAnims::Type curAnim;
    PlayerAnims::Type curSubAnim;
    sead::FixedSafeString<PACKBUFSIZE> curAnimStr;
    sead::FixedSafeString<PACKBUFSIZE> curSubAnimStr;
    float blendWeights[6] = {};
    float animRate = 0.f;
    bool is2D = false;
    s8 gameMode = -1;
    // Puppet Hack Cap Info
    sead::Vector3f capPos = sead::Vector3f(0.f, 0.f, 0.f);
    sead::Quatf capRot = sead::Quatf(0.f, 0.f, 0.f, 0.f);
    sead::Quatf capQuat = sead::Quatf(0.f, 0.f, 0.f, 1.f);
    sead::FixedSafeString<PACKBUFSIZE> capAnim;
    bool isCapThrow = false;
    bool isHoldThrow = false;
    // Hide and Seek Gamemode Info
    bool isIt = false;
    u8 seconds = 0;
    u16 minutes = 0;
    // Freeze Tag Gamemode Info
    u16 freezeTagScore = 0;
    bool isFreezeTagRunner = true;
    bool isFreezeTagFreeze = false;
    bool isFreezeTagFallenOff = false;  // When runenr falls off and is automatically frozen, this flag is set
    float freezeIconSize = 0.f;
    // Shine Thief Gamemode Info
    u16 shineThiefScore = 0;
    bool isShineThiefHolder = false;  // TRUE = has shine, FALSE = thief (CHANGED FROM isShineThiefRunner)
    bool isShineThiefFallenOff = false;
    float shineThiefIconSize = 0.f;
    u8 shineThiefTeam = 0;  // 0 = NONE, 1 = TEAM_1, 2 = TEAM_2
};