#include "hk/hook/Trampoline.h"

#include "nn/fs/fs_mount.h"
#include "nn/nifm.h"
#include "nn/socket.h"

#include "sead/filedevice/nin/seadNinFileDeviceBaseNin.h"
#include "sead/filedevice/seadFileDeviceMgr.h"
#include "sead/heap/seadExpHeap.h"
#include "sead/heap/seadHeapMgr.h"

#include "al/Library/Memory/HeapUtil.h"
#include "al/Library/System/SystemKit.h"

#include "game/Sequence/SequenceInitInfo.h"
#include "game/System/GameSystem.h"

#include "Imgui.hpp"
#include "saveManager.h"
#include "server/Client.hpp"

static constexpr int socketPoolSize = 6_MB;
static constexpr int socketAllocPoolSize = 128_KB;
static char socketPool[socketPoolSize + socketAllocPoolSize] __attribute__((aligned(4_KB)));
static HkReplace<void> disableSocketInit = [] {};

HkTrampoline gameSystemInit = [](TrampolineStatic(), GameSystem* gameSystem) -> void {
    nn::nifm::Initialize();
    nn::nifm::SubmitNetworkRequest();

    while (nn::nifm::IsNetworkRequestOnHold()) {
    }

    nn::socket::Initialize(socketPool, socketPoolSize, socketAllocPoolSize, 0xE);
    disableSocketInit.installAtSym<"_ZN2nn6socket10InitializeEPvmmi">();

#if DEBUGLOG
    Logger::createInstance();
#endif

    imgui::setup();

    Client::createInstance(gHeap);
    SaveManager::createInstance(gHeap);

    orig(gameSystem);
};

HkTrampoline hakoniwaSequenceInitHook = [](TrampolineStatic(), HakoniwaSequence* sequence,
                                           al::SequenceInitInfo* initInfo) -> void {
    orig(sequence, initInfo);
    // was threadInit (hook for initializing client class)
    al::LayoutInitInfo lytInfo;

    al::initLayoutInitInfo(&lytInfo, sequence->mLayoutKit, 0, sequence->mAudioDirector,
                           initInfo->mSystemInfo->layoutSystem, initInfo->mSystemInfo->messageSystem,
                           initInfo->mSystemInfo->gamePadSystem);

    Client::instance()->init(lytInfo, sequence->mGameDataHolderAccessor);
};

al::LiveActor* createPuppetActorFromFactory(const al::ActorInitInfo& initInfo) {
    sead::ScopedCurrentHeapSetter setter(al::getSceneHeap());

    PuppetActor* newActor = new PuppetActor("PuppetActor");

    if (Client::tryAddPuppet(newActor)) {
        PuppetInfo* curInfo = Client::getLatestInfo();
        if (!curInfo) {
            hk::diag::logLine("[Factory] ERROR: Puppet Info is Null!");
            delete newActor;
            return nullptr;
        } else {
            hk::diag::logLine("[Factory] Creating puppet for player: %s", curInfo->puppetName);

            // set puppet info first before calling init so we can get costume info from the
            // info
            newActor->initOnline(curInfo);
            newActor->init(initInfo);

            hk::diag::logLine("[Factory] Puppet initialized successfully for %s", curInfo->puppetName);
        }
    } else {
        hk::diag::logLine("[Factory] ERROR: Failed to add puppet to client");
        delete newActor;
        return nullptr;
    }

    return newActor;
}

HkTrampoline initActorInitInfoHook = [](TrampolineStatic(), al::ActorInitInfo* initInfo, al::Scene* scene,
                                        al::PlacementInfo* placementInfo, al::LayoutInitInfo* layoutInfo,
                                        al::ActorFactory* actorFactory, al::SceneMsgCtrl* sceneMsgCtrl,
                                        al::GameDataHolderBase* gameDataHolderBase) -> void {
    orig(initInfo, scene, placementInfo, layoutInfo, actorFactory, sceneMsgCtrl, gameDataHolderBase);

    if (!scene || !al::isEqualString(scene->mName.cstr(), "StageScene"))
        return;

    // was stage init hook
    gIsSceneAlive = true;

    Client::sendGameInfPacket(scene);

    for (s32 i = 0; i < (Client::getMaxPlayerCount() - 1); i++) {
        createPuppetActorFromFactory(*initInfo);
    }
};

HkTrampoline mountSdCardHook = [](TrampolineStatic(), sead::FileDeviceMgr* fileDeviceMgr) -> void {
    orig(fileDeviceMgr);

    fileDeviceMgr->mMountedSd = nn::fs::MountSdCardForDebug("sd") == 0;
    sead::NinFileDeviceBase* sdFileDevice = new (gHeap) sead::NinFileDeviceBase("sd", "sd");
    fileDeviceMgr->mount(sdFileDevice);
};

HkTrampoline createHeap = [](TrampolineStatic(), al::SystemKit* systemKit, sead::Heap* rootHeap) -> void {
    orig(systemKit, rootHeap);

    gHeap = sead::ExpHeap::create(2_MB, "SMOOPlusHeap", al::getStationedHeap());
    al::addNamedHeap(gHeap, "SMOOPlusHeap");
};

void installInitHooks() {
    mountSdCardHook.installAtSym<"_ZN4sead13FileDeviceMgrC1Ev">();
    createHeap.installAtSym<"_ZN2al9SystemKit18createMemorySystemEPN4sead4HeapE">();
    gameSystemInit.installAtSym<"_ZN10GameSystem4initEv">();
    hakoniwaSequenceInitHook.installAtSym<"_ZN16HakoniwaSequence4initERKN2al16SequenceInitInfoE">();
    initActorInitInfoHook.installAtSym<"R_ZN2al17initActorInitInfo">();
}