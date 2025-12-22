#include "al/Library/Sequence/Sequence.h"

#include "game/Player/PlayerActorBase.h"
#include "game/System/GameDataHolderAccessor.h"
static bool isInGame = false;

static bool debugMode = false;

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