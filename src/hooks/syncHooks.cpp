#include "hk/hook/Trampoline.h"

#include "al/Library/Nerve/IUseNerve.h"
#include "al/Library/Nerve/NerveUtil.h"

#include "game/Item/ShineInfo.h"
#include "game/MapObj/CheckpointFlag.h"
#include "game/MapObj/MoonRock.h"
#include "game/System/GameDataFunction.h"
#include "game/System/GameDataHolderWriter.h"
#include "game/Util/AchievementUtil.h"

#include "helpers.hpp"
#include "layouts/PlayerEventLog.h"
#include "server/captureSync.hpp"

HkTrampoline registerShineToListHook = [](TrampolineStatic(), Shine* shine) -> void {
    orig(shine);
    if (shine->mShineIdx >= 0) {
        Client::tryRegisterShine(shine);
    }
};

HkTrampoline registerCoinCollectToListHook = [](TrampolineStatic(), void* coinCollectHolder,
                                                CoinCollect* coin) -> void {
    orig(coinCollectHolder, coin);
    Client::tryRegisterCoinCollect(coin);
};

HkTrampoline registerCoinCollect2DToListHook = [](TrampolineStatic(), void* coinCollectHolder,
                                                  CoinCollect2D* coin) -> void {
    orig(coinCollectHolder, coin);
    Client::tryRegisterCoinCollect2D(coin);
};

HkTrampoline sendShinePacketHook = [](TrampolineStatic(), GameDataHolderWriter writer,
                                      ShineInfo* info) -> void {
    if (!GameDataFunction::isGotShine(writer, info)) {
        for (int x = 0; x < 0x400; x++) {
            GameDataFile::HintInfo* curInfo = &writer->getGameDataFile()->getHintList()[x];
            if (info->mStageName == curInfo->stageName && info->mObjId == curInfo->objId) {
                Client::sendShineCollectPacket(curInfo->uniqueId);

                PlayerEventLog::addSelfEvent(PlayerEventLog::SHINE, PlayerEventLog::getShineMessage(
                                                                        curInfo->stageName, curInfo->objId));
            }
        }
    }
    orig(writer, info);
};

HkTrampoline sendToadetteShinePacketHook = [](TrampolineStatic(), GameDataFile* file,
                                              const char* name) -> void {
    if (!rs::checkGetAchievement(file->getGameDataHolder(), name)) {
        for (int i = 0; i < hk::util::arraySize(toadetteMoons); i++) {
            if (strcmp(toadetteMoons[i], name) == 0) {
                Client::sendShineCollectPacket(2000 + i);

                PlayerEventLog::addSelfEvent(PlayerEventLog::SHINE,
                                             PlayerEventLog::getAchievementMessage(name));
            }
        }
        orig(file, name);
    }
};

HkTrampoline sendCoinCollectCollectPacketHook = [](TrampolineStatic(), GameDataFile* file,
                                                   al::PlacementId* placeID) -> void {
    al::StringTmp<128> placeIDString;
    placeID->makeString(&placeIDString);
    Client::sendCoinCollectCollectPacket(placeIDString.cstr(), file->getCurrentWorldIdNoDevelop(),
                                         file->getStageNameCurrent());

    PlayerEventLog::addSelfEvent(PlayerEventLog::PURPLE, worldNames[file->getCurrentWorldIdNoDevelop()]);

    orig(file, placeID);
};

HkTrampoline sendCheckpointGetPacketHook = [](TrampolineStatic(), CheckpointFlag* checkpoint) -> void {
    if (al::isFirstStep(checkpoint)) {
        al::StringTmp<128> placementId = al::makeStringPlacementId(checkpoint->getPlacementId());
        Client::sendCheckpointGetPacket(placementId.cstr());

        PlayerEventLog::addSelfEvent(PlayerEventLog::CHECKPOINT,
                                     PlayerEventLog::getCheckpointMessage(placementId));
    }
    orig(checkpoint);
};

class StageSceneStateSelectMode : public al::IUseNerve {};
HkTrampoline startNewGameHook = [](TrampolineStatic(), StageSceneStateSelectMode* thisPtr) -> void {
    if (al::isStep(thisPtr, 5)) {
        PlayerEventLog::addSelfEvent(PlayerEventLog::START, "");
        Client::sendGameStartPacket();
    }

    orig(thisPtr);
};

HkTrampoline moonRockHook = [](TrampolineStatic(), MoonRock* moonRock) -> void {
    if (al::isFirstStep(moonRock)) {
        Client::sendMoonRockHitPacket(GameDataFunction::getCurrentWorldIdNoDevelop(moonRock));

        PlayerEventLog::addSelfEvent(PlayerEventLog::MOONROCK,
                                     worldNames[GameDataFunction::getCurrentWorldIdNoDevelop(moonRock)]);
    }

    orig(moonRock);
};

HkTrampoline initObjHook = [](TrampolineStatic(), al::ActorInitInfo& initInfo,
                              al::PlacementInfo* placement) -> void {
    al::Sequence* sequence = Client::getSequence();
    if (sequence) {
        auto scene = sequence->mCurrentScene;

        if (!scene || !scene->mIsAlive || !al::isEqualString(scene->mName.cstr(), "StageScene")) {
            return orig(initInfo, placement);
        }
    }

    const char* className;

    if (al::tryGetClassName(&className, *placement) && isInCaptureList(className)) {
        int serverMaxPlayers = Client::getMaxPlayerCount();

        for (size_t i = 0; i < serverMaxPlayers - 1; i++) {
            PuppetActor* curPuppet = Client::getPuppet(i);
            if (curPuppet) {
                const char* hackName = tryConvertName(className);

                // make sure we only make as many unique puppet hack actors as needed
                if (!curPuppet->isInCaptureList(hackName)) {
                    PuppetHackActor* dupliActor =
                        createPuppetHackActor(initInfo, placement, curPuppet->getInfo(), hackName);
                    if (dupliActor) {
                        curPuppet->addCapture(dupliActor, hackName);
                    }
                }
            }
        }
    }
    return orig(initInfo, placement);
};

HkTrampoline initMarioModelActorHook = [](TrampolineStatic(), al::LiveActor* actor,
                                          al::ActorInitInfo& initInfo, char* bodyModel, char* capModel,
                                          al::AudioKeeper* keeper, bool isCloset) -> PlayerCostumeInfo* {
    Client::sendCostumeInfPacket(bodyModel, capModel);
    return orig(actor, initInfo, bodyModel, capModel, keeper, isCloset);
};

void installSyncHooks() {
    // Shine Syncing
    sendShinePacketHook
        .installAtSym<"_ZN16GameDataFunction11setGotShineE20GameDataHolderWriterPK9ShineInfo">();
    sendToadetteShinePacketHook.installAtSym<"_ZN12GameDataFile14getAchievementEPKc">();
    registerShineToListHook.installAtSym<"_ZN5Shine18initAfterPlacementEv">();

    // CoinCollect Syncing
    sendCoinCollectCollectPacketHook.installAtSym<"_ZN12GameDataFile14addCoinCollectEPKN2al11PlacementIdE">();
    registerCoinCollectToListHook
        .installAtSym<"_ZN17CoinCollectHolder19registerCoinCollectEP11CoinCollect">();
    registerCoinCollect2DToListHook
        .installAtSym<"_ZN17CoinCollectHolder21registerCoinCollect2DEP13CoinCollect2D">();

    // CheckpointFlag Syncing
    sendCheckpointGetPacketHook.installAtSym<"_ZN14CheckpointFlag6exeGetEv">();

    // StartGame Syncing
    startNewGameHook.installAtSym<"_ZN20SceneStateSelectMode9exeDecideEv">();

    // MoonRock Syncing
    moonRockHook.installAtSym<"_ZN8MoonRock11exeReactionEv">();

    // Outfit Syncing
    initMarioModelActorHook.installAtSym<"R_ZN14PlayerFunction19initMarioModelActor">();

    // Capture Syncing
    // initObjHook.installAtSym<"_ZN2al31createPlacementActorFromFactoryERKNS_13ActorInitInfoEPKNS_13PlacementInfoE">();
    // patch GpuMemAllocator::init to have more space; fixes some crashes with capture sync
    // hk::hook::a64::assemble<"mov w2, {}">().arg(0x1d00000 + 0x0200000).installAtMainOffset(0x00878708);
    // hk::hook::a64::assemble<"mov w2, {}">().arg(0x3e00000 + 0x0200000).installAtMainOffset(0x00878730);
    // hk::hook::a64::assemble<"mov w2, {}">().arg(0x0300000 + 0x0200000).installAtMainOffset(0x0087875c);
}
