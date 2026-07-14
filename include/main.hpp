#pragma once
#include "hk/prim/traits/Integer.h"

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
inline int pInfSendTimer = 0;
inline int gameInfSendTimer = 0;
inline int debugPuppetIndex = 0;
inline int pageIndex = 0;
constexpr int maxPages = 4;

constexpr size extraRAMAmount = 4_MB;
static_assert((extraRAMAmount / 1_MB) % 2 == 0, "Extra RAM amount must be multiple of 2");

void installSyncHooks();
void installModMenuHooks();
void installInitHooks();
void installQolHooks();
void installOtherHooks();

void drawMain(al::Sequence* seq);
void updatePlayerInfo(GameDataHolderAccessor holder, PlayerActorBase* playerBase, bool isYukimaru);
