#include "saveManager.h"

#include <nn/fs.h>

#include <sead/heap/seadHakkunHeap.h>

#include <heap/seadFrameHeap.h>
#include <stream/seadRamStream.h>

#include "fsHelper.h"
#include "layouts/PlayerEventLog.h"
#include "Library/Thread/FunctorV0M.h"
#include "Library/Yaml/ByamlUtil.h"
#include "Scene/StageSceneStateModConfig.hpp"
#include "server/Client.hpp"
#include "System/GameConfigData.h"

SaveManager* SaveManager::sInstance = nullptr;

SaveManager::SaveManager()
    : mThread("SaveManagerThread", al::FunctorV0M(this, &SaveManager::write), 0, 0x4000, {0}) {}

void SaveManager::startThread(GameConfigData* config) {
    if (config)
        mConfig = *config;

    if (mThread.isDone())
        mThread.start();
}

void SaveManager::write() {
    if (mHeap) {
        mHeap->destroy();
        mHeap = nullptr;
    }

    mHeap = sead::FrameHeap::create(10_KB, "SaveManagerHeap", sead::HakkunHeap::sInstance);

    al::ByamlWriter writer(mHeap, false);

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
    writer.addInt("CameraStickSensitivityLevel", mConfig.mCameraStickSensitivityLevel);
    writer.addBool("IsCameraReverseInputH", mConfig.mIsCameraReverseInputH);
    writer.addBool("IsCameraReverseInputV", mConfig.mIsCameraReverseInputV);
    writer.addBool("IsValidCameraGyro", mConfig.mIsValidCameraGyro);
    writer.addInt("CameraGyroSensitivityLevel", mConfig.mCameraGyroSensitivityLevel);
    writer.addBool("IsUseOpenListAdditionalButton", mConfig.mIsUseOpenListAdditionalButton);
    writer.addBool("IsPadRumble", mConfig.mIsValidPadRumble);
    writer.addInt("PadRumbleLevel", mConfig.mPadRumbleLevel);
    writer.pop();

    writer.pop();
    u32 size = writer.calcPackSize();
    mBuffer = (u8*)mHeap->alloc(size);
    sead::RamStreamSrc ramStream(mBuffer, size);
    sead::WriteStream writeStream;
    writeStream.setSrc(&ramStream);
    writeStream.setMode(sead::Stream::Modes::Binary);
    writer.write(&writeStream);
    if (!FsHelper::isFileExist(sSettingsPath))
        nn::fs::CreateDirectory(sModFolder);
    FsHelper::writeFileToPath(mBuffer, size, sSettingsPath);

    mHeap->destroy();
    mHeap = nullptr;
}

void SaveManager::read(GameConfigData* config) {
    if (!FsHelper::isFileExist(sSettingsPath)) {
        nn::fs::CreateDirectory(sModFolder);
        return;
    }

    const char* serverIP = "";
    s32 serverPort = 0;
    bool serverHidden = false;
    bool capCollision = false;
    bool capBounce = false;
    bool playerCollision = true;
    bool playerBounce = true;
    bool costumeDoorsUnlocked = true;
    bool lowLatency = false;
    s32 logLife = 0;
    bool log = true;
    bool shineCount = true;
    bool music = true;

    FsHelper::LoadData data = {.path = sSettingsPath};
    FsHelper::loadFileFromPath(data);

    if (!data.buffer)
        return;

    al::ByamlIter rootIter((u8*)data.buffer);
    al::ByamlIter smooIter;
    al::ByamlIter gameIter;

    if (al::tryGetByamlIterByKey(&smooIter, rootIter, "SMOOData")) {
        if (al::tryGetByamlString(&serverIP, smooIter, "ServerIP"))
            Client::setLastUsedIP(serverIP);
        if (al::tryGetByamlS32(&serverPort, smooIter, "ServerPort"))
            Client::setLastUsedPort(serverPort);
        if (al::tryGetByamlBool(&serverHidden, smooIter, "ServerHidden"))
            Client::setServerHidden(serverHidden);
        if (al::tryGetByamlBool(&capCollision, smooIter, "CapCollision"))
            StageSceneStateModConfig::setCapCollisionEnabled(capCollision);
        if (al::tryGetByamlBool(&capBounce, smooIter, "CapBounce"))
            StageSceneStateModConfig::setCapBounceEnabled(capBounce);
        if (al::tryGetByamlBool(&playerCollision, smooIter, "PlayerCollision"))
            StageSceneStateModConfig::setPuppetCollisionEnabled(playerCollision);
        if (al::tryGetByamlBool(&playerBounce, smooIter, "PlayerBounce"))
            StageSceneStateModConfig::setPuppetBounceEnabled(playerBounce);
        if (al::tryGetByamlBool(&costumeDoorsUnlocked, smooIter, "CostumeDoorsUnlocked"))
            StageSceneStateModConfig::setCostumeDoorsUnlocked(costumeDoorsUnlocked);
        if (al::tryGetByamlBool(&lowLatency, smooIter, "LowLatency"))
            StageSceneStateModConfig::setLowLatencyEnabled(lowLatency);
        if (al::tryGetByamlS32(&logLife, smooIter, "LogLife"))
            StageSceneStateModConfig::setSpeedrunLogLife((StageSceneStateModConfig::SpeedrunLogLife)logLife);
        if (al::tryGetByamlBool(&log, smooIter, "Log"))
            PlayerEventLog::setShow(log);
        if (al::tryGetByamlBool(&shineCount, smooIter, "ShineCount"))
            StageSceneStateModConfig::setShineCountEnabled(shineCount);
        if (al::tryGetByamlBool(&music, smooIter, "Music")) {
            if (Client::isMusicDisabled() != !music) {
                Client::toggleMusicDisabled();
            }
        }
    }
    if (al::tryGetByamlIterByKey(&gameIter, rootIter, "GameConfigData")) {
        al::tryGetByamlS32(&config->mCameraStickSensitivityLevel, gameIter, "CameraStickSensitivityLevel");
        al::tryGetByamlBool(&config->mIsCameraReverseInputH, gameIter, "IsCameraReverseInputH");
        al::tryGetByamlBool(&config->mIsCameraReverseInputV, gameIter, "IsCameraReverseInputV");
        al::tryGetByamlBool(&config->mIsValidCameraGyro, gameIter, "IsValidCameraGyro");
        al::tryGetByamlS32(&config->mCameraGyroSensitivityLevel, gameIter, "CameraGyroSensitivityLevel");
        al::tryGetByamlBool(&config->mIsUseOpenListAdditionalButton, gameIter,
                            "IsUseOpenListAdditionalButton");
        al::tryGetByamlBool(&config->mIsValidPadRumble, gameIter, "IsPadRumble");
        al::tryGetByamlS32(&config->mPadRumbleLevel, gameIter, "PadRumbleLevel");
    }

    free(data.buffer);
}