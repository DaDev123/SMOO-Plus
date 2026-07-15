#pragma once

#include "sead/basis/seadNew.h"
#include "sead/heap/seadHeap.h"

#include "al/Library/Sequence/Sequence.h"

#include "game/Player/PlayerActorBase.h"
#include "game/System/GameDataHolderAccessor.h"

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

void installSyncHooks();
void installModMenuHooks();
void installInitHooks();
void installQolHooks();
void installOtherHooks();

void drawMain(al::Sequence* seq);
void updatePlayerInfo(GameDataHolderAccessor holder, PlayerActorBase* playerBase, bool isYukimaru);
