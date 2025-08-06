#include "speedboot/SpeedbootLoad.hpp"
#include "server/DeltaTime.hpp"
#include "al/util.hpp"
#include "al/util/LayoutUtil.h"
#include "al/util/NerveUtil.h"
#include "game/WorldList/WorldResourceLoader.h"
#include "gfx/seadColor.h"
#include "logger.hpp"
#include "math/seadMathCalcCommon.h"
#include "server/Client.hpp"
#include "game/SaveData/SaveDataAccessFunction.h"

// Forward declare the nerve classes
namespace {
    NERVE_HEADER(SpeedbootLoad, Appear)
    NERVE_HEADER(SpeedbootLoad, Wait)
    NERVE_HEADER(SpeedbootLoad, Decrease)
    NERVE_HEADER(SpeedbootLoad, End)
}

SpeedbootLoad::SpeedbootLoad(
        WorldResourceLoader* resourceLoader,
        const al::LayoutInitInfo& initInfo,
        float autoCloseAfter
    )
    : al::LayoutActor("SpeedbootLoad"), worldResourceLoader(resourceLoader) {
    al::initLayoutActor(this, initInfo, "SpeedbootLoad", nullptr);
    initNerve(&nrvSpeedbootLoadAppear, 0);
    appear();
}

void SpeedbootLoad::exeAppear() {
    if (al::isFirstStep(this))
        al::startAction(this, "Appear", nullptr);
    if (al::isActionEnd(this, nullptr))
        al::setNerve(this, &nrvSpeedbootLoadWait);
}

void SpeedbootLoad::exeWait() {
    if (al::isActionEnd(this, nullptr))
        al::setNerve(this, &nrvSpeedbootLoadDecrease);
}

void SpeedbootLoad::exeDecrease() {
    al::setPaneString(this, "TxtTip", u"Change Server IP/Port: Press +", 0);
    #if EMU
    al::setPaneString(this, "TxtName", u"SMOO-Plus for Emulator", 0);
    #else
    al::setPaneString(this, "TxtName", u"SMOO-Plus for Switch", 0);
    #endif
    
    if (al::isPadTriggerPlus(-1)) {
        Logger::log("Plus button pressed. Opening keyboard for IP and Port input.\n");

        if (Client* client = Client::get()) {
            Client::getKeyboard()->setHeaderText(u"Set Server IP");
            Client::getKeyboard()->setSubText(u"Enter IP Address below:");
            Client::openKeyboardIP();

            Client::getKeyboard()->setHeaderText(u"Set Server Port");
            Client::getKeyboard()->setSubText(u"Enter Port below:");
            Client::openKeyboardPort();

            SaveDataAccessFunction::startSaveDataWrite(client->getHolder().mData);

            mHasConnected = true;
            mConnectionDisplayTimer = 5.0f;

            al::setPaneLocalAlpha(this, "ConnectingGlobe", 1.0f);
            al::setPaneLocalAlpha(this, "ConnectingArrows", 1.0f);
            al::setPaneVtxColor(this, "TxtConnecting", sead::Color4u8{255, 255, 255, 255});
            
            // Make the text bigger and center it on screen
            al::setPaneLocalScale(this, "TxtConnecting", sead::Vector2f{1.5f, 1.5f}); // 1.5x bigger
            al::setPaneLocalTrans(this, "TxtConnecting", sead::Vector3f{0.0f, 0.0f, 0.0f}); // Center position
            al::setPaneString(this, "TxtConnecting", u"You changed the server and have to restart the game now.", 0);
        }
    }

    mProgression = worldResourceLoader->calcLoadPercent() / 100.0f;

    sead::Color4u8 fixedColor = {0, 0, 139, 255};
    mRotTime += 0.03f;
    float rotation = cosf(mRotTime) * 5;

    sead::WFormatFixedSafeString<0x40> debugString(u"Display Time: %.02f\nSin Value: %.02f", mRotTime, rotation);
    al::setPaneString(this, "TxtDebug", debugString.cstr(), 0);

    float arrowRotation = -mRotTime * 75.0f;
    al::setPaneLocalRotate(this, "ConnectingArrows", { 0.0f, 0.0f, arrowRotation });

    // --- Connection status display ---
    if (!mHasConnected) {
        if (Client::isSocketActive()) {
            mHasConnected = true;
            mConnectionDisplayTimer = 0.0f;
            al::setPaneString(this, "TxtConnecting", u"Server Connected!", 0);
        } else {
            mDotTimer += 1.0f / 60.0f;
            if (mDotTimer >= 0.5f) {
                mConnectingDotState = (mConnectingDotState + 1) % 4;
                mDotTimer = 0.0f;
            }

            const char16_t* dots[] = {
                u"Connecting to server",
                u"Connecting to server.",
                u"Connecting to server..",
                u"Connecting to server..."
            };
            al::setPaneString(this, "TxtConnecting", dots[mConnectingDotState], 0);
        }
        al::setPaneVtxColor(this, "TxtConnecting", sead::Color4u8{255, 255, 255, 255});
    } else {
        mConnectionDisplayTimer += 1.0f / 60.0f;
        if (mConnectionDisplayTimer > 2.0f) {
            float fade = sead::Mathf::clamp(1.0f - (mConnectionDisplayTimer - 2.0f) / 0.4f, 0.0f, 1.0f);
            u8 alpha = static_cast<u8>(fade * 255);
            al::setPaneVtxColor(this, "TxtConnecting", sead::Color4u8{255, 255, 255, alpha});
            al::setPaneLocalAlpha(this, "ConnectingGlobe", fade);
            al::setPaneLocalAlpha(this, "ConnectingArrows", fade);
        }
    }

    // --- Loading dots animation ---
    mDotTimer += 1.0f / 60.0f;
    if (mDotTimer >= 0.5f) {
        mLoadingDotState = (mLoadingDotState + 1) % 4;
        mDotTimer = 0.0f;
    }

    const char16_t* loadingDots[] = {
        u"Loading",
        u"Loading.",
        u"Loading..",
        u"Loading..."
    };
    al::setPaneString(this, "TxtLoading", loadingDots[mLoadingDotState], 0);
    al::setPaneVtxColor(this, "TxtLoading", sead::Color4u8{255, 255, 255, 255});

    // --- Progress visuals ---
    if (mProgression < 1.0f) {
        al::setPaneLocalScale(this, "PicBar", { mProgression, 2.f });
        al::setPaneLocalScale(this, "PicBarFill", { 30.f, 1.f });

        al::setPaneVtxColor(this, "PicBar", fixedColor);
        al::setPaneVtxColor(this, "PicBarFill", fixedColor);

        al::setPaneLocalRotate(this, "PicMoon", { 0.f, 0.f, rotation });
        al::setPaneLocalRotate(this, "Arrows", { 0.f, 0.f, arrowRotation });
        al::setPaneLocalRotate(this, "PicBG", { 0.f, 0.f, mRotTime * -3.f });
    }

    if (mProgression > 1.0f)
        al::setNerve(this, &nrvSpeedbootLoadEnd);
}

void SpeedbootLoad::exeEnd() {
    if (al::isFirstStep(this))
        al::startAction(this, "End", nullptr);
    if (al::isActionEnd(this, nullptr))
        kill();
}

namespace {
    NERVE_IMPL(SpeedbootLoad, Appear)
    NERVE_IMPL(SpeedbootLoad, Wait)
    NERVE_IMPL(SpeedbootLoad, Decrease)
    NERVE_IMPL(SpeedbootLoad, End)
}