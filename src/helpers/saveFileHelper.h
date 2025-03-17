#pragma once

#include "heap/seadDisposer.h"
#include "Menu.h"

class SaveFileHelper {
    SEAD_SINGLETON_DISPOSER(SaveFileHelper)

private:
    char* mSettingsPath = "sd:/BTT-Studio/Settings.bin";
    char* mtpPath = "sd:/BTT-Studio/Teleport-States.bin";

public:
    SaveFileHelper() = default;

    void saveSettings();
    void loadSettings();
    void saveTeleport(btt::Menu::TpState* states, size_t count);
    void loadTeleport(btt::Menu::TpState* states, size_t count);
};
