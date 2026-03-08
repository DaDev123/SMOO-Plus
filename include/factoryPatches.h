#pragma once

#include "al/Library/Camera/CameraPoserFactory.h"
#include "al/Library/Camera/CreateCameraPoserFunction.h"
#include "al/Library/LiveActor/ActorFactory.h"
#include "al/Library/LiveActor/CreateActorFunction.h"

#include "actors/PuppetActor.h"
#include "actors/PuppetHackActor.h"
#include "cameras/CameraPoserActorSpectate.h"
#include "cameras/CameraPoserCustom.h"
#include "Scene/Twists/Fludd/actors/FluddBase.hpp"
#include "Scene/Twists/Fludd/actors/FluddHover.hpp"
#include "Scene/Twists/Fludd/actors/FluddRocket.hpp"
#include "Scene/Twists/Fludd/actors/FluddTurbo.hpp"
#include "server/freeze/FreezePlayerBlock.h"

__attribute((used)) static al::NameToCreator<al::ActorCreatorFunction> sCustomActorFactoryEntries[] = {
    {"FreezePlayerBlock", &al::createActorFunction<FreezePlayerBlock>}, {"PuppetActor", &al::createActorFunction<PuppetActor>},
    {"PuppetHackActor", &al::createActorFunction<PuppetHackActor>},     {"FluddBase", &al::createActorFunction<FluddBase>},
    {"FluddHover", &al::createActorFunction<ca::FluddHover>},           {"FluddRocket", &al::createActorFunction<ca::FluddRocket>},
    {"FluddTurbo", &al::createActorFunction<ca::FluddTurbo>},
};

__attribute((used)) static al::NameToCreator<al::CameraPoserCreatorFunction> sCustomCameraFactoryEntries[] = {
    {"CameraPoserCustom", &al::createCameraPoserFunction<cc::CameraPoserCustom>},
    {"CameraPoserActorSpectate", &al::createCameraPoserFunction<cc::CameraPoserActorSpectate>},
};

void insertCustomThingsInFactory();