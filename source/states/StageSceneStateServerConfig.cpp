#include "game/StageScene/StageSceneStateServerConfig.hpp"

#include <cstdlib>
#include <cstring>
#include <math.h>
#include <vector>

#include "al/string/StringTmp.h"
#include "al/util.hpp"

#include "game/SaveData/SaveDataAccessFunction.h"

#include "logger.hpp"
#include <filedevice/seadFileDeviceMgr.h>

#include "rs/util/InputUtil.h"

#include "sead/basis/seadNew.h"
#include "sead/container/seadPtrArray.h"
#include "sead/container/seadSafeArray.h"
#include "sead/prim/seadSafeString.h"
#include "sead/prim/seadStringUtil.h"

#include "nn/fs.h"

#include "server/Client.hpp"
#include "server/gamemode/GameModeBase.hpp"
#include "server/gamemode/GameModeFactory.hpp"
#include "server/gamemode/GameModeManager.hpp"

// Static settings initialization
bool StageSceneStateServerConfig::sCapAttackEnabled = false;
bool StageSceneStateServerConfig::sCapReceiveEnabled = false;
bool StageSceneStateServerConfig::sPuppetAttackEnabled = true;
bool StageSceneStateServerConfig::sPuppetReceiveEnabled = true;
bool StageSceneStateServerConfig::sCostumeDoorsUnlocked = true;
bool StageSceneStateServerConfig::sLowLatencyEnabled = true;

// Public server list structure implementation
PublicServer::PublicServer() : name(nullptr), ip(nullptr), port(0) {}

PublicServer::PublicServer(const char* n, const char* i, int p) {
    name = strdup(n);
    ip = strdup(i);
    port = p;
}

PublicServer::~PublicServer() {
    if (name) free(name);
    if (ip) free(ip);
}

PublicServer::PublicServer(const PublicServer& other) {
    name = other.name ? strdup(other.name) : nullptr;
    ip = other.ip ? strdup(other.ip) : nullptr;
    port = other.port;
}

PublicServer& PublicServer::operator=(const PublicServer& other) {
    if (this != &other) {
        if (name) free(name);
        if (ip) free(ip);
        name = other.name ? strdup(other.name) : nullptr;
        ip = other.ip ? strdup(other.ip) : nullptr;
        port = other.port;
    }
    return *this;
}

// Helper function to load servers from file using sead FileDevice
static std::vector<PublicServer> loadPublicServersFromFile() {
    std::vector<PublicServer> servers;
    
    const char* filePath = "NetworkData/list.txt";
    
    // Check if file exists first
    if (!al::isExistFile(filePath)) {
        // File doesn't exist
        servers.push_back(PublicServer("ERROR: list.txt not found", "", 0));
        return servers;
    }
    
    // File exists, try to load it using sead FileDevice
    sead::FileDevice* device = sead::FileDeviceMgr::instance()->getDefaultFileDevice();
    if (!device) {
        // No default device
        servers.push_back(PublicServer("ERROR: No File Device", "", 0));
        return servers;
    }
    
    // Try to load the file
    sead::FileDevice::LoadArg loadArg;
    loadArg.path = filePath;
    loadArg.buffer = nullptr;
    loadArg.buffer_size = 0;
    loadArg.heap = al::getCurrentHeap();
    loadArg.alignment = 0x20;
    loadArg.div_size = 0;
    loadArg.assert_on_alloc_fail = false;
    
    u8* fileData = device->tryLoad(loadArg);
    
    if (!fileData || loadArg.read_size == 0) {
        // Failed to load
        servers.push_back(PublicServer("ERROR: Load Failed", "", 0));
        return servers;
    }
    
    // Copy to null-terminated buffer
    char* buffer = new char[loadArg.read_size + 1];
    memcpy(buffer, fileData, loadArg.read_size);
    buffer[loadArg.read_size] = '\0';
    
    // Free the loaded data if needed
    if (loadArg.need_unload && fileData) {
        delete[] fileData;
    }
    
    // Parse line by line
    char* savePtr = nullptr;
    char* line = strtok_r(buffer, "\n\r", &savePtr);
    
    while (line != nullptr) {
        // Trim leading whitespace
        while (*line == ' ' || *line == '\t') line++;
        
        // Skip empty lines and comments
        if (strlen(line) == 0 || line[0] == '#') {
            line = strtok_r(nullptr, "\n\r", &savePtr);
            continue;
        }
        
        // Parse format: "Name|IP|Port"
        char* savePtr2 = nullptr;
        char* name = strtok_r(line, "|", &savePtr2);
        char* ip = strtok_r(nullptr, "|", &savePtr2);
        char* portStr = strtok_r(nullptr, "|", &savePtr2);
        
        if (name && ip && portStr) {
            // Trim whitespace from each field
            while (*name == ' ' || *name == '\t') name++;
            while (*ip == ' ' || *ip == '\t') ip++;
            while (*portStr == ' ' || *portStr == '\t') portStr++;
            
            int port = atoi(portStr);
            if (port > 0 && port < 65536) {
                servers.push_back(PublicServer(name, ip, port));
            }
        }
        
        line = strtok_r(nullptr, "\n\r", &savePtr);
    }
    
    // Free buffer
    delete[] buffer;
    
    // If no servers were loaded from file
    if (servers.empty()) {
        servers.push_back(PublicServer("ERROR: Empty or Invalid File", "", 0));
    }
    
    return servers;
}

StageSceneStateServerConfig::StageSceneStateServerConfig(
    const char* name,
    al::Scene* scene,
    const al::LayoutInitInfo& initInfo,
    FooterParts* footerParts,
    GameDataHolder* dataHolder,
    bool
) : al::HostStateBase<al::Scene>(name, scene) {
    mFooterParts = footerParts;
    mGameDataHolder = dataHolder;
    mMsgSystem = initInfo.getMessageSystem();
    mInput = new InputSeparator(mHost, true);

    // Load public servers from file
    mPublicServers = loadPublicServersFromFile();
    mPublicServerCount = mPublicServers.size();

    // === MAIN MENU ===
    mMainOptions = new SimpleLayoutMenu("ServerConfigMenu", "OptionSelect", initInfo, 0, false);
    mMainOptionsList = new CommonVerticalList(mMainOptions, initInfo, true);
    al::setPaneString(mMainOptions, "TxtOption", u"Mod Configuration", 0);
    mMainOptionsList->unkInt1 = 1;
    mMainOptionsList->initDataNoResetSelected(mMainMenuOptionsCount);
    mMainMenuOptions = new sead::SafeArray<sead::WFixedSafeString<0x200>, mMainMenuOptionsCount>();
    updateMainMenuOptions();
    mMainOptionsList->addStringData(mMainMenuOptions->mBuffer, "TxtContent");

    // === NETWORK SETTINGS ===
    mNetworkMenu = new SimpleLayoutMenu("NetworkMenu", "OptionSelect", initInfo, 0, false);
    mNetworkList = new CommonVerticalList(mNetworkMenu, initInfo, true);
    al::setPaneString(mNetworkMenu, "TxtOption", u"Network Settings", 0);
    mNetworkList->unkInt1 = 1;
    mNetworkList->initDataNoResetSelected(3);
    mNetworkOptions = new sead::SafeArray<sead::WFixedSafeString<0x200>, 3>();
    updateNetworkSettingsOptions();
    mNetworkList->addStringData(mNetworkOptions->mBuffer, "TxtContent");

    // === PUBLIC SERVER SELECTION ===
    mPublicServerMenu = new SimpleLayoutMenu("PublicServerMenu", "OptionSelect", initInfo, 0, false);
    mPublicServerList = new CommonVerticalList(mPublicServerMenu, initInfo, true);
    al::setPaneString(mPublicServerMenu, "TxtOption", u"Server List (romfs/NetworkData/list.txt)", 0);
    mPublicServerList->unkInt1 = 1;
    mPublicServerList->initDataNoResetSelected(mPublicServerCount);
    
    // Dynamically create server list options
    mPublicServerOptions = new sead::WFixedSafeString<0x200>[mPublicServerCount];
    
    for (int i = 0; i < mPublicServerCount; i++) {
        mPublicServerOptions[i].convertFromMultiByteString(
            mPublicServers[i].name,
            strlen(mPublicServers[i].name)
        );
    }
    
    mPublicServerList->addStringData(mPublicServerOptions, "TxtContent");

    // === GAMEPLAY SETTINGS ===
    mGameplayMenu = new SimpleLayoutMenu("GameplayMenu", "OptionSelect", initInfo, 0, false);
    mGameplayList = new CommonVerticalList(mGameplayMenu, initInfo, true);
    al::setPaneString(mGameplayMenu, "TxtOption", u"Gameplay Settings", 0);
    mGameplayList->unkInt1 = 1;
    mGameplayList->initDataNoResetSelected(4);
    mGameplayOptions = new sead::SafeArray<sead::WFixedSafeString<0x200>, 4>();
    updateGameplaySettingsOptions();
    mGameplayList->addStringData(mGameplayOptions->mBuffer, "TxtContent");

    // === PLAYER COLLISION SETTINGS (submenu) ===
    mPlayerCollisionMenu = new SimpleLayoutMenu("PlayerCollisionMenu", "OptionSelect", initInfo, 0, false);
    mPlayerCollisionList = new CommonVerticalList(mPlayerCollisionMenu, initInfo, true);
    al::setPaneString(mPlayerCollisionMenu, "TxtOption", u"Player Collision", 0);
    mPlayerCollisionList->unkInt1 = 1;
    mPlayerCollisionList->initDataNoResetSelected(4);
    mPlayerCollisionOptions = new sead::SafeArray<sead::WFixedSafeString<0x200>, 4>();
    updatePlayerCollisionOptions();
    mPlayerCollisionList->addStringData(mPlayerCollisionOptions->mBuffer, "TxtContent");

    // === GAME MODE SETTINGS ===
    mGameModeSettingsMenu = new SimpleLayoutMenu("GameModeSettingsMenu", "OptionSelect", initInfo, 0, false);
    mGameModeSettingsList = new CommonVerticalList(mGameModeSettingsMenu, initInfo, true);
    al::setPaneString(mGameModeSettingsMenu, "TxtOption", u"Game Mode", 0);
    mGameModeSettingsList->unkInt1 = 1;
    mGameModeSettingsList->initDataNoResetSelected(3);
    mGameModeSettingsOptions = new sead::SafeArray<sead::WFixedSafeString<0x200>, 3>();
    updateGameModeSettingsOptions();
    mGameModeSettingsList->addStringData(mGameModeSettingsOptions->mBuffer, "TxtContent");

    // === GAME MODE SELECTION ===
    mModeSelect = new SimpleLayoutMenu("GameModeSelectMenu", "OptionSelect", initInfo, 0, false);
    mModeSelectList = new CommonVerticalList(mModeSelect, initInfo, true);
    al::setPaneString(mModeSelect, "TxtOption", u"Select Game Mode", 0);

    const int modeCount = GameModeFactory::getModeCount();
    mModeSelectList->initDataNoResetSelected(modeCount);

    sead::SafeArray<sead::WFixedSafeString<0x200>, modeCount>* modeSelectOptions =
        new sead::SafeArray<sead::WFixedSafeString<0x200>, modeCount>();

    for (size_t i = 0; i < modeCount; i++) {
        const char* modeName = GameModeFactory::getModeName(i);
        modeSelectOptions->mBuffer[i].convertFromMultiByteString(modeName, strlen(modeName));
    }

    mModeSelectList->addStringData(modeSelectOptions->mBuffer, "TxtContent");

    // === GAME MODE CONFIG ===
    GameModeConfigMenuFactory factory("GameModeConfigFactory");
    for (int mode = 0; mode < factory.getMenuCount(); mode++) {
        GameModeEntry& entry = mGamemodeConfigMenus[mode];
        const char* name = factory.getMenuName(mode);
        entry.mMenu = factory.getCreator(name)(name);
        entry.mLayout = new SimpleLayoutMenu("GameModeConfigMenu", "OptionSelect", initInfo, 0, false);
        entry.mList = new CommonVerticalList(entry.mLayout, initInfo, true);
        al::setPaneString(entry.mLayout, "TxtOption", u"Mode Configuration", 0);
        entry.mList->initDataNoResetSelected(entry.mMenu->getMenuSize());
        entry.mList->addStringData(entry.mMenu->getStringData(), "TxtContent");
    }

    // === TWISTS SETTINGS (now submenu of Game Mode Settings) ===
    mTwistsMenu = new SimpleLayoutMenu("TwistsMenu", "OptionSelect", initInfo, 0, false);
    mTwistsList = new CommonVerticalList(mTwistsMenu, initInfo, true);
    al::setPaneString(mTwistsMenu, "TxtOption", u"Twists & Modifiers", 0);
    mTwistsList->unkInt1 = 1;
    mTwistsList->initDataNoResetSelected(3);
    mTwistsOptions = new sead::SafeArray<sead::WFixedSafeString<0x200>, 3>();
    updateTwistsOptions();
    mTwistsList->addStringData(mTwistsOptions->mBuffer, "TxtContent");

    mCurrentList = mMainOptionsList;
    mCurrentMenu = mMainOptions;
}

StageSceneStateServerConfig::~StageSceneStateServerConfig() {
    // Clean up dynamically allocated server options
    if (mPublicServerOptions) {
        delete[] mPublicServerOptions;
    }
}

void StageSceneStateServerConfig::init() {
    initNerve(&nrvStageSceneStateServerConfigMainMenu, 0);
}

void StageSceneStateServerConfig::appear(void) {
    mCurrentMenu->startAppear("Appear");
    al::NerveStateBase::appear();
}

void StageSceneStateServerConfig::kill(void) {
    if (Client::hasServerChanged()) {
        Client::showUIMessage(u"Server changed. Please restart the game.");
        
        for (int i = 0; i < 180; i++) {
            nn::os::YieldThread();
        }
        
        Client::hideUIMessage();
    }
    
    mCurrentMenu->startEnd("End");
    al::NerveStateBase::kill();
}

al::MessageSystem* StageSceneStateServerConfig::getMessageSystem(void) const {
    return mMsgSystem;
}

// === UPDATE METHODS ===

void StageSceneStateServerConfig::updateMainMenuOptions() {
    mMainMenuOptions->mBuffer[Network_SETTINGS].copy(u"Network Settings");
    mMainMenuOptions->mBuffer[GAMEPLAY_SETTINGS].copy(u"Gameplay Settings");
    mMainMenuOptions->mBuffer[GAMEMODE_SETTINGS].copy(u"Game Mode Settings");
}

void StageSceneStateServerConfig::updateNetworkSettingsOptions() {
    mNetworkOptions->mBuffer[0].copy(u"Browse Server List");
    mNetworkOptions->mBuffer[1].copy(u"Custom Server IP");
    mNetworkOptions->mBuffer[2].copy(u"Custom Server Port");
}

void StageSceneStateServerConfig::updateGameplaySettingsOptions() {
    mGameplayOptions->mBuffer[0].copy(u"Player Collision Settings");
    mGameplayOptions->mBuffer[1].copy(
        sCostumeDoorsUnlocked ? u"Unlock Costume Doors (ON)" : u"Unlock Costume Doors (OFF)"
    );
    mGameplayOptions->mBuffer[2].copy(
        sLowLatencyEnabled ? u"Low Latency Mode (ON)" : u"Low Latency Mode (OFF)"
    );
    mGameplayOptions->mBuffer[3].copy(
        Client::isMusicDisabled() ? u"In-Game Music (OFF)" : u"In-Game Music (ON)"
    );
}

void StageSceneStateServerConfig::updatePlayerCollisionOptions() {
    mPlayerCollisionOptions->mBuffer[0].copy(
        sCapAttackEnabled ? u"Cap Collision (ON)" : u"Cap Collision (OFF)"
    );
    mPlayerCollisionOptions->mBuffer[1].copy(
        sCapReceiveEnabled ? u"Cap Bouncing (ON)" : u"Cap Bouncing (OFF)"
    );
    mPlayerCollisionOptions->mBuffer[2].copy(
        sPuppetAttackEnabled ? u"Player Collision (ON)" : u"Player Collision (OFF)"
    );
    mPlayerCollisionOptions->mBuffer[3].copy(
        sPuppetReceiveEnabled ? u"Player Bouncing (ON)" : u"Player Bouncing (OFF)"
    );
}

void StageSceneStateServerConfig::updateGameModeSettingsOptions() {
    const char* gameModeName = GameModeFactory::getModeName(GameModeManager::instance()->getGameMode());
    int size = strlen(gameModeName) + 25;
    char gameModeText[size];
    snprintf(gameModeText, size, "Configure %s", gameModeName);
    mGameModeSettingsOptions->mBuffer[0].convertFromMultiByteString(gameModeText, size);
    
    mGameModeSettingsOptions->mBuffer[1].copy(u"Twists & Modifiers");
    
    mGameModeSettingsOptions->mBuffer[2].copy(
        GameModeManager::instance()->getInfo<GameModeInfoBase>()
        ? u"Change Mode (reload required)"
        : u"Change Game Mode"
    );
}

void StageSceneStateServerConfig::updateTwistsOptions() {
    mTwistsOptions->mBuffer[0].copy(
        TwistsConfig::isCappyDisableEnabled() ? u"Disable Cappy (OFF)" : u"Disable Cappy (ON)"
    );
    mTwistsOptions->mBuffer[1].copy(
        TwistsConfig::isIcePhysicsEnabled() ? u"Ice Physics (ON)" : u"Ice Physics (OFF)"
    );
    mTwistsOptions->mBuffer[2].copy(u"More twists coming soon...");
}

// === MENU EXECUTION METHODS ===

void StageSceneStateServerConfig::exeMainMenu() {
    if (al::isFirstStep(this)) {
        activateInput();
    }

    mInput->update();
    mCurrentList->update();

    if (mInput->isTriggerUiUp()) {
        mCurrentList->up();
    }

    if (mInput->isTriggerUiDown()) {
        mCurrentList->down();
    }

    if (rs::isTriggerUiCancel(mHost)) {
        kill();
    }

    if (rs::isTriggerUiDecide(mHost)) {
        deactivateInput();
    }

    if (mIsDecideConfig && mCurrentList->isDecideEnd()) {
        switch (mCurrentList->mCurSelected) {
            case Network_SETTINGS:
                al::setNerve(this, &nrvStageSceneStateServerConfigNetworkSettings);
                break;
            case GAMEPLAY_SETTINGS:
                al::setNerve(this, &nrvStageSceneStateServerConfigGameplaySettings);
                break;
            case GAMEMODE_SETTINGS:
                al::setNerve(this, &nrvStageSceneStateServerConfigGameModeSettings);
                break;
            default:
                kill();
                break;
        }
    }
}

void StageSceneStateServerConfig::exeNetworkSettings() {
    if (al::isFirstStep(this)) {
        mCurrentList = mNetworkList;
        mCurrentMenu = mNetworkMenu;
        subMenuStart();
    }

    subMenuUpdate();

    if (mIsDecideConfig && mCurrentList->isDecideEnd()) {
        switch (mCurrentList->mCurSelected) {
            case 0: // Select Public Server
                al::setNerve(this, &nrvStageSceneStateServerConfigPublicServerSelect);
                break;
            case 1: // Custom Server IP
                al::setNerve(this, &nrvStageSceneStateServerConfigOpenKeyboardIP);
                break;
            case 2: // Custom Server Port
                al::setNerve(this, &nrvStageSceneStateServerConfigOpenKeyboardPort);
                break;
        }
    }
}

void StageSceneStateServerConfig::exePublicServerSelect() {
    if (al::isFirstStep(this)) {
        mCurrentList = mPublicServerList;
        mCurrentMenu = mPublicServerMenu;
        subMenuStart();
    }

    subMenuUpdate();

    if (mIsDecideConfig && mCurrentList->isDecideEnd()) {
        int selected = mCurrentList->mCurSelected;
        if (selected >= 0 && selected < mPublicServerCount) {
            Client::setServerIP(mPublicServers[selected].ip);
            Client::setServerPort(mPublicServers[selected].port);
            
            endSubMenuToParent(mNetworkMenu, mNetworkList);
        }
    }
}

void StageSceneStateServerConfig::exeOpenKeyboardIP() {
    if (al::isFirstStep(this)) {
        mCurrentList->deactivate();

        Client::getKeyboard()->setHeaderText(u"Enter Server IP Address");
        Client::getKeyboard()->setSubText(u"");

        bool isSave = Client::openKeyboardIP();

        al::startHitReaction(mCurrentMenu, "リセット", 0);

        if (isSave) {
            al::setNerve(this, &nrvStageSceneStateServerConfigSaveData);
        } else {
            al::setNerve(this, &nrvStageSceneStateServerConfigNetworkSettings);
        }
    }
}

void StageSceneStateServerConfig::exeOpenKeyboardPort() {
    if (al::isFirstStep(this)) {
        mCurrentList->deactivate();

        Client::getKeyboard()->setHeaderText(u"Enter Server Port");
        Client::getKeyboard()->setSubText(u"");

        bool isSave = Client::openKeyboardPort();

        al::startHitReaction(mCurrentMenu, "リセット", 0);

        if (isSave) {
            al::setNerve(this, &nrvStageSceneStateServerConfigSaveData);
        } else {
            al::setNerve(this, &nrvStageSceneStateServerConfigNetworkSettings);
        }
    }
}

void StageSceneStateServerConfig::exeGameplaySettings() {
    if (al::isFirstStep(this)) {
        mCurrentList = mGameplayList;
        mCurrentMenu = mGameplayMenu;
        subMenuStart();
    }

    subMenuUpdate();

    if (mIsDecideConfig && mCurrentList->isDecideEnd()) {
        switch (mCurrentList->mCurSelected) {
            case 0: // Player Collision Settings
                al::setNerve(this, &nrvStageSceneStateServerConfigPlayerCollisionSettings);
                return;
            case 1: // Unlock Costume Doors
                sCostumeDoorsUnlocked = !sCostumeDoorsUnlocked;
                break;
            case 2: // Low Latency Mode
                sLowLatencyEnabled = !sLowLatencyEnabled;
                break;
            case 3: // Music Toggle
                Client::toggleMusicDisabled();
                break;
        }

        updateGameplaySettingsOptions();
        mGameplayList->initDataNoResetSelected(4);
        mGameplayList->addStringData(mGameplayOptions->mBuffer, "TxtContent");
        mGameplayList->updateParts();
        activateInput();
    }
}

void StageSceneStateServerConfig::exePlayerCollisionSettings() {
    if (al::isFirstStep(this)) {
        mCurrentList = mPlayerCollisionList;
        mCurrentMenu = mPlayerCollisionMenu;
        subMenuStart();
    }

    subMenuUpdate();

    if (mIsDecideConfig && mCurrentList->isDecideEnd()) {
        switch (mCurrentList->mCurSelected) {
            case 0:
                sCapAttackEnabled = !sCapAttackEnabled;
                break;
            case 1:
                sCapReceiveEnabled = !sCapReceiveEnabled;
                break;
            case 2:
                sPuppetAttackEnabled = !sPuppetAttackEnabled;
                break;
            case 3:
                sPuppetReceiveEnabled = !sPuppetReceiveEnabled;
                break;
        }

        updatePlayerCollisionOptions();
        mPlayerCollisionList->initDataNoResetSelected(4);
        mPlayerCollisionList->addStringData(mPlayerCollisionOptions->mBuffer, "TxtContent");
        mPlayerCollisionList->updateParts();
        activateInput();
    }
}

void StageSceneStateServerConfig::exeGameModeSettings() {
    if (al::isFirstStep(this)) {
        mCurrentList = mGameModeSettingsList;
        mCurrentMenu = mGameModeSettingsMenu;
        subMenuStart();
    }

    subMenuUpdate();

    if (mIsDecideConfig && mCurrentList->isDecideEnd()) {
        switch (mCurrentList->mCurSelected) {
            case 0: // Configure Current Mode
                al::setNerve(this, &nrvStageSceneStateServerConfigGameModeConfig);
                return;
            case 1: // Twists & Modifiers
                al::setNerve(this, &nrvStageSceneStateServerConfigTwistsSettings);
                return;
            case 2: // Change Game Mode
                al::setNerve(this, &nrvStageSceneStateServerConfigGameModeSelect);
                return;
        }
    }
}

void StageSceneStateServerConfig::exeGameModeConfig() {
    if (al::isFirstStep(this)) {
        int currentMode = GameModeManager::instance()->getGameMode();
        
        GameModeEntry* foundEntry = nullptr;
        
        if (currentMode >= 0 && currentMode < mGamemodeConfigMenus.size()) {
            foundEntry = &mGamemodeConfigMenus[currentMode];
        }
        
        if (!foundEntry) {
            endSubMenuToParent(mGameModeSettingsMenu, mGameModeSettingsList);
            return;
        }
        
        mGamemodeConfigMenu = foundEntry;
        mGamemodeConfigMenu->mList->initDataNoResetSelected(mGamemodeConfigMenu->mMenu->getMenuSize());
        mGamemodeConfigMenu->mList->addStringData(mGamemodeConfigMenu->mMenu->getStringData(), "TxtContent");
        mCurrentList = mGamemodeConfigMenu->mList;
        mCurrentMenu = mGamemodeConfigMenu->mLayout;
        subMenuStart();
    }
    
    subMenuUpdate();
    
    if (mIsDecideConfig && mCurrentList->isDecideEnd()) {
        if (mGamemodeConfigMenu && mGamemodeConfigMenu->mMenu) {
            GameModeConfigMenu::UpdateAction action = mGamemodeConfigMenu->mMenu->updateMenu(mCurrentList->mCurSelected);
            
            switch (action) {
                case GameModeConfigMenu::UpdateAction::CLOSE: {
                    endSubMenu();
                    break;
                }
                case GameModeConfigMenu::UpdateAction::REFRESH: {
                    subMenuRefresh();
                    break;
                }
                case GameModeConfigMenu::UpdateAction::NOOP: {
                    activateInput();
                    break;
                }
            }
        }
    }
}

void StageSceneStateServerConfig::subMenuRefresh() {
    if (mGamemodeConfigMenu && mGamemodeConfigMenu->mMenu && mGamemodeConfigMenu->mList) {
        mGamemodeConfigMenu->mList->initDataNoResetSelected(mGamemodeConfigMenu->mMenu->getMenuSize());
        mGamemodeConfigMenu->mList->addStringData(mGamemodeConfigMenu->mMenu->getStringData(), "TxtContent");
        mGamemodeConfigMenu->mList->updateParts();
    }
    activateInput();
}

void StageSceneStateServerConfig::exeGameModeSelect() {
    if (al::isFirstStep(this)) {
        mCurrentList = mModeSelectList;
        mCurrentMenu = mModeSelect;
        subMenuStart();
    }

    subMenuUpdate();

    if (mIsDecideConfig && mCurrentList->isDecideEnd()) {
        int selectedMode = mCurrentList->mCurSelected;
        Logger::log("Setting Mode: %d\n", selectedMode);
        GameModeManager::instance()->setMode(static_cast<GameMode>(selectedMode));
        
        // Update the game mode settings menu to reflect the new mode
        updateGameModeSettingsOptions();
        mGameModeSettingsList->initDataNoResetSelected(3);
        mGameModeSettingsList->addStringData(mGameModeSettingsOptions->mBuffer, "TxtContent");
        mGameModeSettingsList->updateParts();
        
        endSubMenuToParent(mGameModeSettingsMenu, mGameModeSettingsList);
    }
}

void StageSceneStateServerConfig::exeTwistsSettings() {
    if (al::isFirstStep(this)) {
        mCurrentList = mTwistsList;
        mCurrentMenu = mTwistsMenu;
        subMenuStart();
    }

    subMenuUpdate();

    if (mIsDecideConfig && mCurrentList->isDecideEnd()) {
        switch (mCurrentList->mCurSelected) {
            case 0:
                TwistsConfig::toggleCappyDisable();
                break;
            case 1:
                TwistsConfig::toggleIcePhysics();
                break;
            case 2:
                // Future twist option
                break;
        }

        updateTwistsOptions();
        mTwistsList->initDataNoResetSelected(3);
        mTwistsList->addStringData(mTwistsOptions->mBuffer, "TxtContent");
        mTwistsList->updateParts();
        activateInput();
        mIsDecideConfig = false;
    }
}

void StageSceneStateServerConfig::exeSaveData() {
    if (al::isFirstStep(this)) {
        SaveDataAccessFunction::startSaveDataWrite(mGameDataHolder);
    }

    if (SaveDataAccessFunction::updateSaveDataAccess(mGameDataHolder, false)) {
        al::startHitReaction(mCurrentMenu, "リセット", 0);
        al::setNerve(this, &nrvStageSceneStateServerConfigMainMenu);
    }
}

// === HELPER METHODS ===

void StageSceneStateServerConfig::subMenuStart() {
    mCurrentList->deactivate();
    // Don't kill the menu - just end it gracefully to avoid flash
    mCurrentMenu->startEnd("End");
    activateInput();
    mCurrentMenu->startAppear("Appear");
}

void StageSceneStateServerConfig::subMenuUpdate() {
    mInput->update();
    mCurrentList->update();

    if (mInput->isTriggerUiUp()) {
        mCurrentList->up();
    }

    if (mInput->isTriggerUiDown()) {
        mCurrentList->down();
    }

    // Only allow cancel if we're not in the process of deciding
    if (rs::isTriggerUiCancel(mHost) && !mIsDecideConfig) {
        // Check submenus first (specific to general order)
        if (mCurrentMenu == mPlayerCollisionMenu) {
            endSubMenuToParent(mGameplayMenu, mGameplayList);
        } else if (mCurrentMenu == mPublicServerMenu) {
            endSubMenuToParent(mNetworkMenu, mNetworkList);
        } else if (mCurrentMenu == mModeSelect) {
            endSubMenuToParent(mGameModeSettingsMenu, mGameModeSettingsList);
        } else if (mCurrentMenu == mTwistsMenu) {
            endSubMenuToParent(mGameModeSettingsMenu, mGameModeSettingsList);
        } else if (mGamemodeConfigMenu != nullptr && mCurrentMenu == mGamemodeConfigMenu->mLayout) {
            endSubMenuToParent(mGameModeSettingsMenu, mGameModeSettingsList);
        } else if (mCurrentMenu == mNetworkMenu || 
                   mCurrentMenu == mGameplayMenu || 
                   mCurrentMenu == mGameModeSettingsMenu) {
            // These are top-level submenus, go back to main
            endSubMenu();
        } else {
            // Default fallback
            endSubMenu();
        }
    }

    if (rs::isTriggerUiDecide(mHost)) {
        deactivateInput();
    }
}

void StageSceneStateServerConfig::endSubMenu() {
    mCurrentList->deactivate();
    // Use startEnd instead of kill for smoother transition
    mCurrentMenu->startEnd("End");

    mCurrentList = mMainOptionsList;
    mCurrentMenu = mMainOptions;

    mCurrentMenu->startAppear("Appear");
    al::startHitReaction(mCurrentMenu, "リセット", 0);
    al::setNerve(this, &nrvStageSceneStateServerConfigMainMenu);
}

void StageSceneStateServerConfig::endSubMenuToParent(SimpleLayoutMenu* parentMenu, CommonVerticalList* parentList) {
    mCurrentList->deactivate();
    mCurrentMenu->startEnd("End");

    mCurrentList = parentList;
    mCurrentMenu = parentMenu;

    // Remove this - let the nerve's first step handle it
    // mCurrentMenu->startAppear("Appear");
    
    activateInput();
    mIsDecideConfig = false;
    
    // Set appropriate nerve based on parent menu
    if (parentMenu == mGameplayMenu) {
        al::setNerve(this, &nrvStageSceneStateServerConfigGameplaySettings);
    } else if (parentMenu == mNetworkMenu) {
        al::setNerve(this, &nrvStageSceneStateServerConfigNetworkSettings);
    } else if (parentMenu == mGameModeSettingsMenu) {
        al::setNerve(this, &nrvStageSceneStateServerConfigGameModeSettings);
    }
}

void StageSceneStateServerConfig::activateInput() {
    mInput->reset();
    mCurrentList->activate();
    mCurrentList->appearCursor();
    mIsDecideConfig = false;
}

void StageSceneStateServerConfig::deactivateInput() {
    al::startHitReaction(mCurrentMenu, "決定", 0);
    mCurrentList->endCursor();
    mCurrentList->decide();
    mIsDecideConfig = true;
}

// === STATIC GETTERS ===

bool StageSceneStateServerConfig::isCapAttackEnabled() {
    return sCapAttackEnabled;
}

bool StageSceneStateServerConfig::isCapReceiveEnabled() {
    return sCapReceiveEnabled;
}

bool StageSceneStateServerConfig::isPuppetAttackEnabled() {
    return sPuppetAttackEnabled;
}

bool StageSceneStateServerConfig::isPuppetReceiveEnabled() {
    return sPuppetReceiveEnabled;
}

bool StageSceneStateServerConfig::isCostumeDoorsUnlocked() {
    return sCostumeDoorsUnlocked;
}

bool StageSceneStateServerConfig::isLowLatencyEnabled() {
    return sLowLatencyEnabled;
}

// === NERVE IMPLEMENTATIONS ===

namespace {
    NERVE_IMPL(StageSceneStateServerConfig, MainMenu)
    NERVE_IMPL(StageSceneStateServerConfig, NetworkSettings)
    NERVE_IMPL(StageSceneStateServerConfig, PublicServerSelect)
    NERVE_IMPL(StageSceneStateServerConfig, OpenKeyboardIP)
    NERVE_IMPL(StageSceneStateServerConfig, OpenKeyboardPort)
    NERVE_IMPL(StageSceneStateServerConfig, GameplaySettings)
    NERVE_IMPL(StageSceneStateServerConfig, PlayerCollisionSettings)
    NERVE_IMPL(StageSceneStateServerConfig, GameModeSettings)
    NERVE_IMPL(StageSceneStateServerConfig, GameModeConfig)
    NERVE_IMPL(StageSceneStateServerConfig, GameModeSelect)
    NERVE_IMPL(StageSceneStateServerConfig, TwistsSettings)
    NERVE_IMPL(StageSceneStateServerConfig, SaveData)
}