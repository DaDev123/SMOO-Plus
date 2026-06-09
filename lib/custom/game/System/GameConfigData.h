#pragma once

#include "hk/prim/traits/Integer.h"

#include <basis/seadTypes.h>
#include <heap/seadFrameHeap.h>
#include <heap/seadHeap.h>
#include <heap/seadHeapMgr.h>

#include "fsHelper.h"
#include "layouts/PlayerEventLog.h"
#include "Library/Yaml/Writer/ByamlWriter.h"
#include "Scene/StageSceneStateModConfig.hpp"
#include "server/Client.hpp"
#include "stream/seadRamStream.h"
#include "stream/seadStream.h"
#include "System/ByamlSave.h"

constexpr const char* sSettingsPath = "sd:/SMOO-Plus/settings.byml";
constexpr const char* sModFolder = "sd:/SMOO-Plus";

namespace al {
class IUseSceneObjHolder;
class LayoutActor;
class Scene;
}  // namespace al

class GameConfigData : public ByamlSave {
public:
    GameConfigData();
    void init();
    bool isCameraReverseInputH() const;
    void onCameraReverseInputH();
    void offCameraReverseInputH();
    bool isCameraReverseInputV() const;
    void onCameraReverseInputV();
    void offCameraReverseInputV();
    s32 getCameraStickSensitivityLevel() const;
    void setCameraStickSensitivityLevel(s32 value);
    bool isValidCameraGyro() const;
    void validateCameraGyro();
    void invalidateCameraGyro();
    s32 getCameraGyroSensitivityLevel() const;
    void setCameraGyroSensitivityLevel(s32 value);
    bool isUseOpenListAdditionalButton() const;
    void onUseOpenListAdditionalButton();
    void offUseOpenListAdditionalButton();
    bool isValidPadRumble() const;
    void validatePadRumble();
    void invalidatePadRumble();
    s32 getPadRumbleLevel() const;
    void setPadRumbleLevel(s32 value);
    void write(al::ByamlWriter* writer) override;
    void read(const al::ByamlIter& save) override;

    // custom function
    typedef void (GameConfigData::*SaveWriteThreadFunc)(void);

    void writeToSd() {
        sead::FrameHeap* frameHeap =
            sead::FrameHeap::create(10_KB, "SaveWriteHeap", Client::getClientHeap(), 8,
                                    sead::Heap::HeapDirection::cHeapDirection_Forward, false);

        sead::ScopedCurrentHeapSetter heapSetter(frameHeap);

        al::ByamlWriter writer(frameHeap, false);

        const char* serverIP = Client::getCurrentIP();
        const s32 serverPort = Client::getCurrentPort();
        const bool serverHidden = Client::isServerHidden();
        const bool capCollision = StageSceneStateModConfig::isCapCollisionEnabled();
        const bool capBounce = StageSceneStateModConfig::isCapBounceEnabled();
        const bool playerCollision = StageSceneStateModConfig::isPuppetCollisionEnabled();
        const bool playerBounce = StageSceneStateModConfig::isPuppetBounceEnabled();
        const bool costumeDoorsUnlocked = StageSceneStateModConfig::isCostumeDoorsUnlocked();
        const bool lowLatency = StageSceneStateModConfig::isLowLatencyEnabled();
        const s32 logLife = StageSceneStateModConfig::getSpeedrunLogLife();
        const bool log = PlayerEventLog::isShow();
        const bool shineCount = StageSceneStateModConfig::isShineCountEnabled();
        const bool music = !Client::isMusicDisabled();

        writer.pushHash();
        writer.pushHash("SMOOData");
        if (serverIP) {
            writer.addString("ServerIP", serverIP);
        } else {
            writer.addString("ServerIP", "127.0.0.1");
        }

        if (serverPort) {
            writer.addInt("ServerPort", serverPort);
        } else {
            writer.addInt("ServerPort", 0);
        }

        writer.addBool("ServerHidden", serverHidden);
        writer.addBool("CapCollision", capCollision);
        writer.addBool("CapBounce", capBounce);
        writer.addBool("PlayerCollision", playerCollision);
        writer.addBool("PlayerBounce", playerBounce);
        writer.addBool("CostumeDoorsUnlocked", costumeDoorsUnlocked);
        writer.addBool("LowLatency", lowLatency);
        writer.addInt("LogLife", logLife);
        writer.addBool("Log", log);
        writer.addBool("ShineCount", shineCount);
        writer.addBool("Music", music);
        writer.pop();

        writer.pushHash("GameConfigData");
        writer.addInt("CameraStickSensitivityLevel", mCameraStickSensitivityLevel);
        writer.addBool("IsCameraReverseInputH", mIsCameraReverseInputH);
        writer.addBool("IsCameraReverseInputV", mIsCameraReverseInputV);
        writer.addBool("IsValidCameraGyro", mIsValidCameraGyro);
        writer.addInt("CameraGyroSensitivityLevel", mCameraGyroSensitivityLevel);
        writer.addBool("IsUseOpenListAdditionalButton", mIsUseOpenListAdditionalButton);
        writer.addBool("IsPadRumble", mIsValidPadRumble);
        writer.addInt("PadRumbleLevel", mPadRumbleLevel);
        writer.pop();

        writer.pop();
        u32 size = writer.calcPackSize();
        u8 buffer[size];
        sead::RamStreamSrc ramStream(&buffer, sizeof(buffer));
        sead::WriteStream writeStream;
        writeStream.setSrc(&ramStream);
        writeStream.setMode(sead::Stream::Modes::Binary);
        writer.write(&writeStream);
        FsHelper::writeFileToPath(buffer, size, sSettingsPath);

        frameHeap->freeAll();
        frameHeap->destroy();
    }

public:
    s32 mCameraStickSensitivityLevel = -1;
    bool mIsCameraReverseInputH = false;
    bool mIsCameraReverseInputV = false;
    bool mIsValidCameraGyro = true;
    s32 mCameraGyroSensitivityLevel = -1;
    bool mIsUseOpenListAdditionalButton = false;
    bool mIsValidPadRumble = true;
    s32 mPadRumbleLevel = 0;
};

namespace rs {
GameConfigData* getGameConfigData(const al::LayoutActor*);
void saveGameConfigData(const al::LayoutActor*);
void applyGameConfigData(al::Scene*, const GameConfigData*);
bool isUseOpenListAdditionalButton(const al::IUseSceneObjHolder*);
}  // namespace rs
