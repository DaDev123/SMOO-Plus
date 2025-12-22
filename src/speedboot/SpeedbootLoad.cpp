#include "speedboot/SpeedbootLoad.hpp"

#include "al/Library/Layout/LayoutActionFunction.h"
#include "al/Library/Layout/LayoutActorUtil.h"
#include "al/Library/Layout/LayoutInitInfo.h"
#include "al/Library/Nerve/NerveUtil.h"

#include "game/Sequence/HakoniwaSequence.h"
#include "game/Sequence/WorldResourceLoader.h"
#include "game/System/GameDataFunction.h"

#include "math/seadMathCalcCommon.h"

// Forward declare the nerve classes
namespace {

// Kingdom name mapping
struct KingdomMapping {
    const char* stageName;
    const char16_t* kingdomName;
};

constexpr KingdomMapping KINGDOM_NAMES[] = {
    {"CapWorldHomeStage", u"Cap Kingdom"},       {"WaterfallWorldHomeStage", u"Cascade Kingdom"}, {"SandWorldHomeStage", u"Sand Kingdom"},
    {"ForestWorldHomeStage", u"Wooded Kingdom"}, {"LakeWorldHomeStage", u"Lake Kingdom"},         {"CloudWorldHomeStage", u"Cloud Kingdom"},
    {"ClashWorldHomeStage", u"Lost Kingdom"},    {"CityWorldHomeStage", u"Metro Kingdom"},        {"SnowWorldHomeStage", u"Snow Kingdom"},
    {"SeaWorldHomeStage", u"Seaside Kingdom"},   {"LavaWorldHomeStage", u"Luncheon Kingdom"},     {"BossRaidWorldHomeStage", u"Ruined Kingdom"},
    {"SkyWorldHomeStage", u"Bowser's Kingdom"},  {"MoonWorldHomeStage", u"Moon Kingdom"},         {"PeachWorldHomeStage", u"Mushroom Kingdom"},
    {"Special1WorldHomeStage", u"Dark Side"},    {"Special2WorldHomeStage", u"Darker Side"}};

constexpr f32 FALLBACK_DURATION = 15.0f;  // 15 seconds for unknown stages
constexpr f32 FPS = 60.0f;
}  // namespace

SpeedbootLoad::SpeedbootLoad(WorldResourceLoader* resourceLoader, const al::LayoutInitInfo& initInfo, HakoniwaSequence* sequence)
    : al::LayoutActor("SpeedbootLoad"), worldResourceLoader(resourceLoader), mSequence(sequence) {
    al::initLayoutActor(this, initInfo, "SpeedbootLoad", nullptr);
    initNerve(&NrvSpeedbootLoad.Appear, 0);

    wipe = new al::WipeSimple("黒フェードシーン情報", "FadeBlack", initInfo, 0);
    wipeSkip = new al::WipeSimple("スキップワイプ", "WipeSkip", initInfo, 0);

    // Set the scale for TxtCurModName to make it smaller (adjust the values as needed)
    al::setPaneLocalScale(this, "TxtCurModName", {0.6f, 0.6f});

    appear();
}

void SpeedbootLoad::exeAppear() {
    if (al::isFirstStep(this))
        al::startAction(this, "Appear", nullptr);
    if (al::isActionEnd(this, nullptr))
        al::setNerve(this, &NrvSpeedbootLoad.Wait);
}

void SpeedbootLoad::exeWait() {
    if (al::isActionEnd(this, nullptr))
        al::setNerve(this, &NrvSpeedbootLoad.Decrease);
}

const char* SpeedbootLoad::getStageName() const {
    if (!mSequence)
        return nullptr;

    // Try sequence's stage name first
    if (!mSequence->mStageName.isEmpty()) {
        return mSequence->mStageName.cstr();
    }

    // Try next stage name
    const char* name = GameDataFunction::getNextStageName(mSequence->mGameDataHolderAccessor);
    if (name && name[0] != '\0')
        return name;

    // Fallback to current stage
    return GameDataFunction::getCurrentStageName(mSequence->mGameDataHolderAccessor);
}

const char16_t* SpeedbootLoad::getKingdomName(const char* stageName) const {
    if (!stageName)
        return u"Unknown Kingdom";

    for (const auto& mapping : KINGDOM_NAMES) {
        if (strcmp(stageName, mapping.stageName) == 0) {
            return mapping.kingdomName;
        }
    }

    return nullptr;  // Unknown stage
}

bool SpeedbootLoad::isKnownStage(const char* stageName) const {
    if (!stageName)
        return false;

    for (const auto& mapping : KINGDOM_NAMES) {
        if (strcmp(stageName, mapping.stageName) == 0) {
            return true;
        }
    }
    return false;
}

void SpeedbootLoad::updateProgressBar() {
    const char* stageName = getStageName();

    // Determine loading method
    if (!isKnownStage(stageName) && !mUsingFallback) {
        mUsingFallback = true;
    }

    // Calculate progression
    if (mUsingFallback) {
        mFallbackTimer += 1.0f / FPS;
        mProgression = sead::Mathf::clamp(mFallbackTimer / FALLBACK_DURATION, 0.0f, 1.0f);
    } else {
        f32 loadPercent = worldResourceLoader->calcLoadPercent();
        mProgression = sead::Mathf::clamp(loadPercent / 100.0f, 0.0f, 1.0f);
    }
}

void SpeedbootLoad::updateUIElements() {
    mRotTime += 0.03f;

    // Update rotating elements
    if (mProgression < 1.0f) {
        f32 moonRotation = cosf(mRotTime) * 5.0f;
        al::setPaneLocalRotate(this, "BloodMoon", {0.0f, 0.0f, moonRotation});
        al::setPaneLocalRotate(this, "BloodMoonStar", {0.0f, 0.0f, mRotTime * -15.0f});
        al::setPaneLocalRotate(this, "PicBG", {0.0f, 0.0f, mRotTime * -3.0f});

        // Animated loading bars
        f32 barOffset = mRotTime * 30.0f;
        al::setPaneLocalTrans(this, "BloodMoonLoadingBar", {-352.0f + barOffset, -342.0f, 0.0f});
        al::setPaneLocalTrans(this, "BloodMoonLoadingBar2", {-352.0f + barOffset - 1920.0f, -342.0f, 0.0f});
    }
}

void SpeedbootLoad::updateTextElements() {
    const char* stageName = getStageName();
    const char16_t* kingdomName = getKingdomName(stageName);

    // Set kingdom name
    if (kingdomName) {
        al::setPaneString(this, "TxtKingdom", kingdomName, 0);
    } else {
        // Convert unknown stage name to wide string
        sead::WFormatFixedSafeString<0x100> fallbackString(u"");
        if (stageName) {
            for (const char* c = stageName; *c != '\0'; c++) {
                fallbackString.appendWithFormat(u"%c", (char16_t)(*c));
            }
        }
        al::setPaneString(this, "TxtKingdom", fallbackString.cstr(), 0);
    }

    // Animated "Loading..." text
    s32 dotCount = ((s32)(mRotTime * 0.8f)) % 4;
    sead::WFormatFixedSafeString<0x20> loadingString(u"Loading");
    for (s32 i = 0; i < dotCount; i++) {
        loadingString.append(u".");
    }
    al::setPaneString(this, "TxtLoading", loadingString.cstr(), 0);

    // Display loading percentage
    f32 displayPercent = sead::Mathf::clamp(mProgression * 100.0f, 0.0f, 100.0f);
    sead::WFormatFixedSafeString<0x20> percentString(u"");
    percentString.appendWithFormat(u"%.0f%%", displayPercent);
    al::setPaneString(this, "TxtLoadingPercent", percentString.cstr(), 0);

    // Debug info
    sead::WFormatFixedSafeString<0x100> debugString(u"Progress: %.1f%%", displayPercent);
    al::setPaneString(this, "TxtDebug", debugString.cstr(), 0);
    al::setPaneString(this, "TxtCurModName", u"Super Mario Odyssey Online - Plus", 0);
}

void SpeedbootLoad::exeDecrease() {
    updateProgressBar();
    updateUIElements();
    updateTextElements();

    // Check if loading complete
    if (mProgression >= 1.0f) {
        wipeSkip->tryStartClose(-1);
        wipe->tryStartClose(60);

        if (wipe->isCloseEnd()) {
            al::setNerve(this, &NrvSpeedbootLoad.End);
        }
    }
}

void SpeedbootLoad::exeEnd() {
    if (al::isFirstStep(this)) {
        kill();
        wipeSkip->startOpen(-1);
        wipe->startOpen(30);
    }
}
