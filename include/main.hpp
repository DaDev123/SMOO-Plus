#pragma once
#include "hk/hook/Replace.h"

#include "al/Library/Sequence/Sequence.h"

#include "game/Player/PlayerActorBase.h"
#include "game/System/GameDataHolderAccessor.h"

#include "basis/seadNew.h"
#include "heap/seadHeap.h"

inline bool isInGame = false;
inline bool debugMode = false;

// ===== GLOBAL VARIABLES =====
inline bool gIsSceneAlive = false;
inline sead::Heap* gHeap = nullptr;
static int pInfSendTimer = 0;
static int gameInfSendTimer = 0;
static int debugPuppetIndex = 0;
static int debugCaptureIndex = 0;
static int pageIndex = 0;
static const int maxPages = 4;
static char chatInput[0x100] = "";

static constexpr int socketPoolSize = 6_MB;
static constexpr int socketAllocPoolSize = 128_KB;
static char socketPool[socketPoolSize + socketAllocPoolSize] __attribute__((aligned(4_KB)));
static HkReplace<void> disableSocketInit = [] {};

static constexpr size extraRAMAmount = 4_MB;
static_assert((extraRAMAmount / 1_MB) % 2 == 0, "Extra RAM amount must be multiple of 2");

void drawMain(al::Sequence* seq);
void updatePlayerInfo(GameDataHolderAccessor holder, PlayerActorBase* playerBase, bool isYukimaru);

constexpr const char* captureNames[] = {"AnagramAlphabetCharacter",
                                        "Byugo",
                                        "Bubble",
                                        "Bull",
                                        "Car",
                                        "ElectricWire",
                                        "JugemFishing",
                                        "Statue",
                                        "Fukankun",
                                        "Yoshi",
                                        "KillerLauncherMagnum",
                                        "KuriboPossessed",
                                        "WanwanBig",  // has sub-actors
                                        "KillerLauncher",
                                        "Koopa",
                                        "Wanwan",  // has sub-actors
                                        "Pukupuku",
                                        "PukupukuSnow",
                                        "Gamane",  // has sub-actors
                                        "FireBrosPossessed",
                                        "PackunFire",
                                        "Frog",
                                        "Kakku",
                                        "Hosui",
                                        "HammerBrosPossessed",
                                        "Megane",
                                        "KaronWing",
                                        "KuriboWing",
                                        "PackunPoison",
                                        "Radicon",
                                        "Tank",
                                        "Tsukkun",
                                        "TRex",
                                        "TRexSleep",
                                        "TRexPatrol",
                                        "Imomu",
                                        "SenobiGeneratePoint"};