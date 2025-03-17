#include "saveFileHelper.h"
#include <cstddef>
#include "fsHelper.h"
#include "settings/SettingsMgr.h"

SEAD_SINGLETON_DISPOSER_IMPL(SaveFileHelper);

void SaveFileHelper::saveSettings() {
    if (!FsHelper::isDirExist("sd:/BTT-Studio")) {
        nn::fs::CreateDirectory("sd:/BTT-Studio");
    }

    FsHelper::writeFileToPath(reinterpret_cast<void*>(&btt::SettingsMgr::instance()->mSettings), sizeof(btt::SettingsMgr::Settings), mSettingsPath);
}

void SaveFileHelper::loadSettings() {
    if (!FsHelper::isFileExist(mSettingsPath)) {
        saveSettings();
        return;
    }

    FsHelper::LoadData data;
    data.path = mSettingsPath;
    FsHelper::loadFileFromPath(data);

    if (data.buffer != nullptr && data.bufSize == sizeof(btt::SettingsMgr::Settings)) {
        btt::SettingsMgr::Settings* configData = reinterpret_cast<btt::SettingsMgr::Settings*>(data.buffer);
        btt::SettingsMgr::instance()->mSettings = *configData;
    }
}

void SaveFileHelper::saveTeleport(btt::Menu::TpState* states, size_t count) {
    if (!FsHelper::isDirExist("sd:/BTT-Studio")) {
        nn::fs::CreateDirectory("sd:/BTT-Studio");
    }

    FsHelper::writeFileToPath(reinterpret_cast<void*>(states), sizeof(btt::Menu::TpState) * count, mtpPath);
}

void SaveFileHelper::loadTeleport(btt::Menu::TpState* states, size_t count) {
    if (!FsHelper::isFileExist(mtpPath)) {
        saveTeleport(states, count);
        return;
    }

    FsHelper::LoadData data;
    data.path = mtpPath;
    FsHelper::loadFileFromPath(data);

    if (data.buffer != nullptr && data.bufSize == sizeof(btt::Menu::TpState) * count) {
        btt::Menu::TpState* loadedStates = reinterpret_cast<btt::Menu::TpState*>(data.buffer);
        for (size_t i = 0; i < count; ++i) {
            states[i] = loadedStates[i];
        }
    }
}
