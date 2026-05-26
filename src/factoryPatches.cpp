#include "factoryPatches.h"

#include "hk/hook/Trampoline.h"

#include "game/Scene/ProjectActorFactory.h"

HkTrampoline actorFactoryHook = [](TrampolineStatic(), ProjectActorFactory* actorFactory) -> void {
    orig(actorFactory);
    s32 customActorEntriesCount = sizeof(sCustomActorFactoryEntries) / sizeof(sCustomActorFactoryEntries[0]);
    al::NameToCreator<al::ActorCreatorFunction>* factoryEntries =
        new al::NameToCreator<al::ActorCreatorFunction>[actorFactory->mNumFactoryEntries + customActorEntriesCount];

    for (s32 i = 0; i < actorFactory->mNumFactoryEntries; i++) {
        factoryEntries[i] = actorFactory->mFactoryEntries[i];
    }
    for (s32 i = 0; i < customActorEntriesCount; i++) {
        factoryEntries[actorFactory->mNumFactoryEntries + i] = sCustomActorFactoryEntries[i];
    }

    actorFactory->mFactoryEntries = factoryEntries;
    actorFactory->mNumFactoryEntries += customActorEntriesCount;
};

void insertCustomThingsInFactory() {
    actorFactoryHook.installAtSym<"_ZN19ProjectActorFactoryC2Ev">();
}
