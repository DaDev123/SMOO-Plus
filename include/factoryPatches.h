#pragma once

#include "al/Library/LiveActor/ActorFactory.h"
#include "al/Library/LiveActor/CreateActorFunction.h"

#include "actors/PuppetActor.h"
#include "actors/PuppetHackActor.h"

__attribute((used)) static al::NameToCreator<al::ActorCreatorFunction> sCustomActorFactoryEntries[] = {

    {"PuppetActor", &al::createActorFunction<PuppetActor>},
    {"PuppetHackActor", &al::createActorFunction<PuppetHackActor>},
};

void insertCustomThingsInFactory();