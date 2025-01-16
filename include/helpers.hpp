#pragma once

#include <string>
#include <cstring>
#include "types.h"

#include "sead/math/seadVector.h"
#include "sead/math/seadQuat.hpp"

#include "al/util.hpp"

#include "logger.hpp"
#include "puppets/PuppetInfo.h"

#include "game/GameData/GameDataFunction.h"

bool isPartOf(const char* w1, const char* w2);

int indexOf(char *w1, char c1);

void logVector(const char* vectorName, sead::Vector3f vector);

void logQuat(const char *quatName, sead::Quatf quat);

sead::Vector3f QuatToEuler(sead::Quatf *quat);

float vecMagnitude(sead::Vector3f const &input);

float quatAngle(sead::Quatf const &q1, sead::Quatf &q2);

bool isInCostumeList(const char *costumeName);

const char *tryGetPuppetCapName(PuppetInfo *info);
const char* tryGetPuppetBodyName(PuppetInfo* info);

const char* tryConvertName(const char* className);

void killMainPlayer(al::LiveActor* actor);
void killMainPlayer(PlayerActorHakoniwa* mainPlayer);

__attribute__((used)) static const char* costumeNames[] = {
"Mario",
    "MarioCaptain",
    "MarioDot3d",
    "Mario64",
    "Mario64Metal",
    "MarioAloha",
    "MarioArmor",
    "MarioBone",
    "MarioClown",
    "MarioColorClassic",
    "MarioColorGold",
    "MarioColorLuigi",
    "MarioColorWaluigi",
    "MarioColorWario",
    "MarioCook",
    "MarioDiddyKong",
    "MarioDoctor",
    "MarioExplorer",
    "MarioFootball",
    "MarioGolf",
    "MarioGunman",
    "MarioHakama",
    "MarioHappi",
    "MarioKing",
    "MarioKoopa",
    "MarioMaker",
    "MarioMechanic",
    "MarioNew3DS",
    "MarioPainter",
    "MarioPeach",
    "MarioPilot",
    "MarioPirate",
    "MarioPoncho",
    "MarioPrimitiveMan",
    "MarioSailor",
    "MarioScientist",
    "MarioShopman",
    "MarioSnowSuit",
    "MarioSpaceSuit",
    "MarioSuit",
    "MarioSwimwear",
    "MarioTailCoat",
    "MarioTuxedo",
    "MarioTuxedo",
    "MarioUnderwear",
    "MarioArmorWestern",
    "MarioRacer",
    "MarioBandman",
    "MarioBatter",
    "MarioConductor",
    "MarioHariet",
    "MarioHigh",
    "MarioRango",
    "MarioRsv",
    "MarioRango",
    "MarioSanta",
    "MarioSatellite",
    "MarioSpewart",
    "MarioSuit",
    "MarioSunshine",
    "MarioTopper",
    "MarioZombie",
    "MarioWooper",
    "MarioLuigiNew",
    "MarioDot3d"
};

struct HackActorName {
    const char *className;
    const char *hackName;
};

// attribute otherwise the build log is spammed with unused warnings
__attribute__((used)) static HackActorName classHackNames[] = {
    {"SenobiGeneratePoint", "Senobi"},
    {"KuriboPossessed", "Kuribo"},
    {"KillerLauncher", "Killer"},
    {"KillerLauncherMagnum", "KillerMagnum"},
    {"FireBrosPossessed", "FireBros"},
    {"HammerBrosPossessed", "HammerBros"},
    {"ElectricWire", "ElectricWireMover"},
    {"TRexSleep", "TRex"},
    {"TRexPatrol", "TRex"},
    {"WanwanBig", "Wanwan"},  // FIXME: this will make chain chomp captures always be the small
                              // variant for syncing
    {"Koopa","KoopaHack"}
};

struct Transform
{
    sead::Vector3f *position;
    sead::Quatf *rotation;
};

// From Boss Room Unity Example
class VisualUtils
{

public:
    /* 
    * @brief Smoothly interpolates towards the parent transform.
    * @param moveTransform The transform to interpolate
    * @param targetTransform The transform to interpolate towards.
    * @param timeDelta Time in seconds that has elapsed, for purposes of interpolation.
    * @param closingSpeed The closing speed in m/s. This is updated by SmoothMove every time it is called, and will drop to 0 whenever the moveTransform has "caught up". 
    * @param maxAngularSpeed The max angular speed to to rotate at, in degrees/s.
    */
    static float SmoothMove(Transform moveTransform, Transform targetTransform, float timeDelta,
                            float closingSpeed, float maxAngularSpeed);

    constexpr static const float k_MinSmoothSpeed = 0.1f;
    constexpr static const float k_TargetCatchupTime = 0.2f;
};