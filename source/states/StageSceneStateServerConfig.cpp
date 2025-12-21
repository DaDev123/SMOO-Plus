#include "game/StageScene/StageSceneStateServerConfig.hpp"

#include <cstdlib>
#include <cstring>
#include <vector>

#include "al/string/StringTmp.h"
#include "al/util.hpp"
#include "game/SaveData/SaveDataAccessFunction.h"
#include "logger.hpp"
#include "rs/util/InputUtil.h"
#include "sead/basis/seadNew.h"
#include "sead/container/seadPtrArray.h"
#include "sead/container/seadSafeArray.h"
#include "sead/prim/seadSafeString.h"
#include "sead/prim/seadStringUtil.h"
#include "server/Client.hpp"
#include "server/gamemode/GameModeFactory.hpp"
#include "server/gamemode/GameModeManager.hpp"
#include "BloodMoon/BloodMoonUtils.hpp"

// ============================================================================
// Static Configuration Variables
// ============================================================================

bool StageSceneStateServerConfig::sCapAttackEnabled = false;
bool StageSceneStateServerConfig::sCapReceiveEnabled = false;
bool StageSceneStateServerConfig::sPuppetAttackEnabled = true;
bool StageSceneStateServerConfig::sPuppetReceiveEnabled = true;
bool StageSceneStateServerConfig::sCostumeDoorsUnlocked = true;
bool StageSceneStateServerConfig::sLowLatencyEnabled = true;

// ============================================================================
// ServerBrowser Implementation
// ============================================================================

ServerBrowser::ServerBrowser() : name(nullptr), ip(nullptr), port(0) {}

ServerBrowser::ServerBrowser(const char* n, const char* i, int p) {
    name = n ? strdup(n) : nullptr;
    ip = i ? strdup(i) : nullptr;
    port = p;
}

ServerBrowser::~ServerBrowser() {
    free(name);
    free(ip);
}

ServerBrowser::ServerBrowser(const ServerBrowser& other) {
    name = other.name ? strdup(other.name) : nullptr;
    ip = other.ip ? strdup(other.ip) : nullptr;
    port = other.port;
}

ServerBrowser& ServerBrowser::operator=(const ServerBrowser& other) {
    if (this != &other) {
        free(name);
        free(ip);
        name = other.name ? strdup(other.name) : nullptr;
        ip = other.ip ? strdup(other.ip) : nullptr;
        port = other.port;
    }
    return *this;
}

// ============================================================================
// Server List Loader
// ============================================================================

static std::vector<ServerBrowser> loadServersFromFile() {
    std::vector<ServerBrowser> servers;
    size_t fileSize = 0;
    u8* fileData = BloodMoon::loadFile("OnlineData/ServerList.txt", &fileSize);
    
    if (!fileData) {
        servers.push_back(ServerBrowser("ERROR: OnlineData/serverlist.txt not found", "", 0));
        return servers;
    }
    
    char* buffer = reinterpret_cast<char*>(fileData);
    char* savePtr = nullptr;
    char* line = strtok_r(buffer, "\n\r", &savePtr);
    
    while (line) {
        // Skip whitespace and comments
        while (*line == ' ' || *line == '\t') line++;
        if (*line == '\0' || *line == '#') {
            line = strtok_r(nullptr, "\n\r", &savePtr);
            continue;
        }
        
        // Parse "Name|IP|Port"
        char* savePtr2 = nullptr;
        char* name = strtok_r(line, "|", &savePtr2);
        char* ip = strtok_r(nullptr, "|", &savePtr2);
        char* portStr = strtok_r(nullptr, "|", &savePtr2);
        
        if (name && ip && portStr) {
            int port = atoi(portStr);
            if (port > 0 && port < 65536) {
                servers.push_back(ServerBrowser(name, ip, port));
            }
        }
        
        line = strtok_r(nullptr, "\n\r", &savePtr);
    }
    
    delete[] fileData;
    
    if (servers.empty()) {
        servers.push_back(ServerBrowser("ERROR: Empty or Invalid File", "", 0));
    }
    
    return servers;
}

// ============================================================================
// Constructor
// ============================================================================

StageSceneStateServerConfig::StageSceneStateServerConfig(
    const char* name, al::Scene* scene, const al::LayoutInitInfo& initInfo,
    FooterParts* footerParts, GameDataHolder* dataHolder, bool
) : al::HostStateBase<al::Scene>(name, scene) {
    
    mFooterParts = footerParts;
    mGameDataHolder = dataHolder;
    mMsgSystem = initInfo.getMessageSystem();
    mInput = new InputSeparator(mHost, true);

    // Load server list
    mServerBrowserServers = loadServersFromFile();
    mServerBrowserCount = mServerBrowserServers.size();

    // Initialize all menus
    initMainMenu(initInfo);
    initNetworkMenu(initInfo);
    initServerBrowserMenu(initInfo);
    initGameplayMenu(initInfo);
    initPlayerCollisionMenu(initInfo);
    initGameModeMenus(initInfo);
    initTwistsMenu(initInfo);

    mCurrentList = mMainOptionsList;
    mCurrentMenu = mMainOptions;
}

StageSceneStateServerConfig::~StageSceneStateServerConfig() {
    delete[] mServerBrowserOptions;
}

// ============================================================================
// Menu Initialization Methods
// ============================================================================

void StageSceneStateServerConfig::initMainMenu(const al::LayoutInitInfo& initInfo) {
    mMainOptions = new SimpleLayoutMenu("ServerConfigMenu", "OptionSelect", initInfo, 0, false);
    mMainOptionsList = new CommonVerticalList(mMainOptions, initInfo, true);
    al::setPaneString(mMainOptions, "TxtOption", u"Mod Configuration", 0);
    mMainOptionsList->unkInt1 = 1;
    mMainOptionsList->initDataNoResetSelected(mMainMenuOptionsCount);
    mMainMenuOptions = new sead::SafeArray<sead::WFixedSafeString<0x200>, mMainMenuOptionsCount>();
    updateMainMenuOptions();
    mMainOptionsList->addStringData(mMainMenuOptions->mBuffer, "TxtContent");
}

void StageSceneStateServerConfig::initNetworkMenu(const al::LayoutInitInfo& initInfo) {
    mNetworkMenu = new SimpleLayoutMenu("NetworkMenu", "OptionSelect", initInfo, 0, false);
    mNetworkList = new CommonVerticalList(mNetworkMenu, initInfo, true);
    al::setPaneString(mNetworkMenu, "TxtOption", u"Network Settings", 0);
    mNetworkList->unkInt1 = 1;
    mNetworkList->initDataNoResetSelected(3);
    mNetworkOptions = new sead::SafeArray<sead::WFixedSafeString<0x200>, 3>();
    updateNetworkSettingsOptions();
    mNetworkList->addStringData(mNetworkOptions->mBuffer, "TxtContent");
}

void StageSceneStateServerConfig::initServerBrowserMenu(const al::LayoutInitInfo& initInfo) {
    mServerBrowserMenu = new SimpleLayoutMenu("ServerBrowserMenu", "OptionSelect", initInfo, 0, false);
    mServerBrowserList = new CommonVerticalList(mServerBrowserMenu, initInfo, true);
    al::setPaneString(mServerBrowserMenu, "TxtOption", u"Server List (OnlineData/ServerList.txt)", 0);
    mServerBrowserList->unkInt1 = 1;
    mServerBrowserList->initDataNoResetSelected(mServerBrowserCount);
    
    mServerBrowserOptions = new sead::WFixedSafeString<0x200>[mServerBrowserCount];
    for (int i = 0; i < mServerBrowserCount; i++) {
        mServerBrowserOptions[i].convertFromMultiByteString(
            mServerBrowserServers[i].name, strlen(mServerBrowserServers[i].name)
        );
    }
    mServerBrowserList->addStringData(mServerBrowserOptions, "TxtContent");
}

void StageSceneStateServerConfig::initGameplayMenu(const al::LayoutInitInfo& initInfo) {
    mGameplayMenu = new SimpleLayoutMenu("GameplayMenu", "OptionSelect", initInfo, 0, false);
    mGameplayList = new CommonVerticalList(mGameplayMenu, initInfo, true);
    al::setPaneString(mGameplayMenu, "TxtOption", u"Gameplay Settings", 0);
    mGameplayList->unkInt1 = 1;
    mGameplayList->initDataNoResetSelected(4);
    mGameplayOptions = new sead::SafeArray<sead::WFixedSafeString<0x200>, 4>();
    updateGameplaySettingsOptions();
    mGameplayList->addStringData(mGameplayOptions->mBuffer, "TxtContent");
}

void StageSceneStateServerConfig::initPlayerCollisionMenu(const al::LayoutInitInfo& initInfo) {
    mPlayerCollisionMenu = new SimpleLayoutMenu("PlayerCollisionMenu", "OptionSelect", initInfo, 0, false);
    mPlayerCollisionList = new CommonVerticalList(mPlayerCollisionMenu, initInfo, true);
    al::setPaneString(mPlayerCollisionMenu, "TxtOption", u"Player Collision", 0);
    mPlayerCollisionList->unkInt1 = 1;
    mPlayerCollisionList->initDataNoResetSelected(4);
    mPlayerCollisionOptions = new sead::SafeArray<sead::WFixedSafeString<0x200>, 4>();
    updatePlayerCollisionOptions();
    mPlayerCollisionList->addStringData(mPlayerCollisionOptions->mBuffer, "TxtContent");
}

void StageSceneStateServerConfig::initGameModeMenus(const al::LayoutInitInfo& initInfo) {
    // Game Mode Settings
    mGameModeSettingsMenu = new SimpleLayoutMenu("GameModeSettingsMenu", "OptionSelect", initInfo, 0, false);
    mGameModeSettingsList = new CommonVerticalList(mGameModeSettingsMenu, initInfo, true);
    al::setPaneString(mGameModeSettingsMenu, "TxtOption", u"Game Mode", 0);
    mGameModeSettingsList->unkInt1 = 1;
    mGameModeSettingsList->initDataNoResetSelected(3);
    mGameModeSettingsOptions = new sead::SafeArray<sead::WFixedSafeString<0x200>, 3>();
    updateGameModeSettingsOptions();
    mGameModeSettingsList->addStringData(mGameModeSettingsOptions->mBuffer, "TxtContent");

    // Mode Selection
    mModeSelect = new SimpleLayoutMenu("GameModeSelectMenu", "OptionSelect", initInfo, 0, false);
    mModeSelectList = new CommonVerticalList(mModeSelect, initInfo, true);
    al::setPaneString(mModeSelect, "TxtOption", u"Select Game Mode", 0);
    
    const int modeCount = GameModeFactory::getModeCount();
    mModeSelectList->initDataNoResetSelected(modeCount);
    
    auto* modeOptions = new sead::SafeArray<sead::WFixedSafeString<0x200>, modeCount>();
    for (size_t i = 0; i < modeCount; i++) {
        const char* modeName = GameModeFactory::getModeName(i);
        modeOptions->mBuffer[i].convertFromMultiByteString(modeName, strlen(modeName));
    }
    mModeSelectList->addStringData(modeOptions->mBuffer, "TxtContent");

    // Mode Config
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
}

void StageSceneStateServerConfig::initTwistsMenu(const al::LayoutInitInfo& initInfo) {
    mTwistsMenu = new SimpleLayoutMenu("TwistsMenu", "OptionSelect", initInfo, 0, false);
    mTwistsList = new CommonVerticalList(mTwistsMenu, initInfo, true);
    al::setPaneString(mTwistsMenu, "TxtOption", u"Twists & Modifiers", 0);
    mTwistsList->unkInt1 = 1;
    mTwistsList->initDataNoResetSelected(3);
    mTwistsOptions = new sead::SafeArray<sead::WFixedSafeString<0x200>, 3>();
    updateTwistsOptions();
    mTwistsList->addStringData(mTwistsOptions->mBuffer, "TxtContent");
}

// ============================================================================
// Lifecycle Methods
// ============================================================================

void StageSceneStateServerConfig::init() {
    initNerve(&nrvStageSceneStateServerConfigMainMenu, 0);
}

void StageSceneStateServerConfig::appear() {
    mCurrentMenu->startAppear("Appear");
    al::NerveStateBase::appear();
}

void StageSceneStateServerConfig::kill() {
    if (Client::hasServerChanged()) {
        Client::showUIMessage(u"Server changed. Please restart the game.");
        for (int i = 0; i < 180; i++) nn::os::YieldThread();
        Client::hideUIMessage();
    }
    mCurrentMenu->startEnd("End");
    al::NerveStateBase::kill();
}

// ============================================================================
// Option Update Methods
// ============================================================================

void StageSceneStateServerConfig::updateMainMenuOptions() {
    mMainMenuOptions->mBuffer[NETWORK_SETTINGS].copy(u"Network Settings");
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
    mGameplayOptions->mBuffer[1].copy(sCostumeDoorsUnlocked ? 
        u"Unlock Costume Doors (ON)" : u"Unlock Costume Doors (OFF)");
    mGameplayOptions->mBuffer[2].copy(sLowLatencyEnabled ? 
        u"Reduce Player Latency (OFF)" : u"Reduce Player Latency (ON)");
    mGameplayOptions->mBuffer[3].copy(Client::isMusicDisabled() ? 
        u"In-Game Music (OFF)" : u"In-Game Music (ON)");
}

void StageSceneStateServerConfig::updatePlayerCollisionOptions() {
    mPlayerCollisionOptions->mBuffer[0].copy(sCapAttackEnabled ? 
        u"Cap Collision (ON)" : u"Cap Collision (OFF)");
    mPlayerCollisionOptions->mBuffer[1].copy(sCapReceiveEnabled ? 
        u"Cap Bouncing (ON)" : u"Cap Bouncing (OFF)");
    mPlayerCollisionOptions->mBuffer[2].copy(sPuppetAttackEnabled ? 
        u"Player Collision (ON)" : u"Player Collision (OFF)");
    mPlayerCollisionOptions->mBuffer[3].copy(sPuppetReceiveEnabled ? 
        u"Player Bouncing (ON)" : u"Player Bouncing (OFF)");
}

void StageSceneStateServerConfig::updateGameModeSettingsOptions() {
    const char* modeName = GameModeFactory::getModeName(GameModeManager::instance()->getGameMode());
    char text[256];
    snprintf(text, sizeof(text), "Configure %s", modeName);
    mGameModeSettingsOptions->mBuffer[0].convertFromMultiByteString(text, strlen(text));
    
    mGameModeSettingsOptions->mBuffer[1].copy(u"Twists & Modifiers");
    mGameModeSettingsOptions->mBuffer[2].copy(
        GameModeManager::instance()->getInfo<GameModeInfoBase>() ? 
        u"Change Mode (reload required)" : u"Change Game Mode");
}

void StageSceneStateServerConfig::updateTwistsOptions() {
    mTwistsOptions->mBuffer[0].copy(TwistsConfig::isCappyDisableEnabled() ? 
        u"Disable Cappy (OFF)" : u"Disable Cappy (ON)");
    mTwistsOptions->mBuffer[1].copy(TwistsConfig::isIcePhysicsEnabled() ? 
        u"Ice Physics (ON)" : u"Ice Physics (OFF)");
    mTwistsOptions->mBuffer[2].copy(u"More twists coming soon...");
}

// ============================================================================
// Menu Execution Methods
// ============================================================================

void StageSceneStateServerConfig::exeMainMenu() {
    if (al::isFirstStep(this)) activateInput();

    handleMenuInput();

    if (rs::isTriggerUiCancel(mHost)) {
        kill();
    }

    if (mIsDecideConfig && mCurrentList->isDecideEnd()) {
        switch (mCurrentList->mCurSelected) {
            case NETWORK_SETTINGS:
                al::setNerve(this, &nrvStageSceneStateServerConfigNetworkSettings);
                break;
            case GAMEPLAY_SETTINGS:
                al::setNerve(this, &nrvStageSceneStateServerConfigGameplaySettings);
                break;
            case GAMEMODE_SETTINGS:
                al::setNerve(this, &nrvStageSceneStateServerConfigGameModeSettings);
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
            case 0: al::setNerve(this, &nrvStageSceneStateServerConfigServerBrowserSelect); break;
            case 1: al::setNerve(this, &nrvStageSceneStateServerConfigOpenKeyboardIP); break;
            case 2: al::setNerve(this, &nrvStageSceneStateServerConfigOpenKeyboardPort); break;
        }
    }
}

void StageSceneStateServerConfig::exeServerBrowserSelect() {
    if (al::isFirstStep(this)) {
        mCurrentList = mServerBrowserList;
        mCurrentMenu = mServerBrowserMenu;
        subMenuStart();
    }

    subMenuUpdate();

    if (mIsDecideConfig && mCurrentList->isDecideEnd()) {
        int selected = mCurrentList->mCurSelected;
        if (selected >= 0 && selected < mServerBrowserCount) {
            Client::setServerIP(mServerBrowserServers[selected].ip);
            Client::setServerPort(mServerBrowserServers[selected].port);
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
        
        if (isSave) {
            // Save directly without transitioning through SaveData nerve
            SaveDataAccessFunction::startSaveDataWrite(mGameDataHolder);
        }
        
        // Re-activate the network menu immediately
        al::startHitReaction(mCurrentMenu, "リセット", 0);
        mCurrentList->activate();
        mCurrentList->appearCursor();
        al::setNerve(this, &nrvStageSceneStateServerConfigNetworkSettings);
    }
}

void StageSceneStateServerConfig::exeOpenKeyboardPort() {
    if (al::isFirstStep(this)) {
        mCurrentList->deactivate();
        Client::getKeyboard()->setHeaderText(u"Enter Server Port");
        Client::getKeyboard()->setSubText(u"");
        bool isSave = Client::openKeyboardPort();
        
        if (isSave) {
            // Save directly without transitioning through SaveData nerve
            SaveDataAccessFunction::startSaveDataWrite(mGameDataHolder);
        }
        
        // Re-activate the network menu immediately
        al::startHitReaction(mCurrentMenu, "リセット", 0);
        mCurrentList->activate();
        mCurrentList->appearCursor();
        al::setNerve(this, &nrvStageSceneStateServerConfigNetworkSettings);
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
        if (mCurrentList->mCurSelected == 0) {
            al::setNerve(this, &nrvStageSceneStateServerConfigPlayerCollisionSettings);
            return;
        }

        // Toggle settings
        switch (mCurrentList->mCurSelected) {
            case 1: sCostumeDoorsUnlocked = !sCostumeDoorsUnlocked; break;
            case 2: sLowLatencyEnabled = !sLowLatencyEnabled; break;
            case 3: Client::toggleMusicDisabled(); break;
        }

        updateGameplaySettingsOptions();
        refreshMenu(mGameplayList, mGameplayOptions->mBuffer, 4);
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
            case 0: sCapAttackEnabled = !sCapAttackEnabled; break;
            case 1: sCapReceiveEnabled = !sCapReceiveEnabled; break;
            case 2: sPuppetAttackEnabled = !sPuppetAttackEnabled; break;
            case 3: sPuppetReceiveEnabled = !sPuppetReceiveEnabled; break;
        }

        updatePlayerCollisionOptions();
        refreshMenu(mPlayerCollisionList, mPlayerCollisionOptions->mBuffer, 4);
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
            case 0: al::setNerve(this, &nrvStageSceneStateServerConfigGameModeConfig); break;
            case 1: al::setNerve(this, &nrvStageSceneStateServerConfigTwistsSettings); break;
            case 2: al::setNerve(this, &nrvStageSceneStateServerConfigGameModeSelect); break;
        }
    }
}

void StageSceneStateServerConfig::exeGameModeConfig() {
    if (al::isFirstStep(this)) {
        int mode = GameModeManager::instance()->getGameMode();
        if (mode < 0 || mode >= mGamemodeConfigMenus.size()) {
            endSubMenuToParent(mGameModeSettingsMenu, mGameModeSettingsList);
            return;
        }
        
        mGamemodeConfigMenu = &mGamemodeConfigMenus[mode];
        mGamemodeConfigMenu->mList->initDataNoResetSelected(mGamemodeConfigMenu->mMenu->getMenuSize());
        mGamemodeConfigMenu->mList->addStringData(mGamemodeConfigMenu->mMenu->getStringData(), "TxtContent");
        mCurrentList = mGamemodeConfigMenu->mList;
        mCurrentMenu = mGamemodeConfigMenu->mLayout;
        subMenuStart();
    }
    
    subMenuUpdate();
    
    if (mIsDecideConfig && mCurrentList->isDecideEnd() && mGamemodeConfigMenu) {
        auto action = mGamemodeConfigMenu->mMenu->updateMenu(mCurrentList->mCurSelected);
        switch (action) {
            case GameModeConfigMenu::UpdateAction::CLOSE: endSubMenu(); break;
            case GameModeConfigMenu::UpdateAction::REFRESH: subMenuRefresh(); break;
            case GameModeConfigMenu::UpdateAction::NOOP: activateInput(); break;
        }
    }
}

void StageSceneStateServerConfig::exeGameModeSelect() {
    if (al::isFirstStep(this)) {
        mCurrentList = mModeSelectList;
        mCurrentMenu = mModeSelect;
        subMenuStart();
    }

    subMenuUpdate();

    if (mIsDecideConfig && mCurrentList->isDecideEnd()) {
        GameModeManager::instance()->setMode(static_cast<GameMode>(mCurrentList->mCurSelected));
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
            case 0: TwistsConfig::toggleCappyDisable(); break;
            case 1: TwistsConfig::toggleIcePhysics(); break;
        }

        updateTwistsOptions();
        refreshMenu(mTwistsList, mTwistsOptions->mBuffer, 3);
        mIsDecideConfig = false;
    }
}

void StageSceneStateServerConfig::exeSaveData() {
    if (al::isFirstStep(this)) {
        SaveDataAccessFunction::startSaveDataWrite(mGameDataHolder);
    }

    if (SaveDataAccessFunction::updateSaveDataAccess(mGameDataHolder, false)) {
        al::startHitReaction(mCurrentMenu, "リセット", 0);
        
        // Return to Network Settings after saving from keyboard input
        mCurrentList->activate();
        mCurrentList->appearCursor();
        al::setNerve(this, &nrvStageSceneStateServerConfigNetworkSettings);
    }
}

// ============================================================================
// Helper Methods
// ============================================================================

void StageSceneStateServerConfig::handleMenuInput() {
    mInput->update();
    mCurrentList->update();
    if (mInput->isTriggerUiUp()) mCurrentList->up();
    if (mInput->isTriggerUiDown()) mCurrentList->down();
    if (rs::isTriggerUiDecide(mHost)) deactivateInput();
}

void StageSceneStateServerConfig::subMenuStart() {
    mCurrentList->deactivate();
    mCurrentMenu->startEnd("End");
    activateInput();
    mCurrentMenu->startAppear("Appear");
}

void StageSceneStateServerConfig::subMenuUpdate() {
    handleMenuInput();

    if (rs::isTriggerUiCancel(mHost) && !mIsDecideConfig) {
        // Determine parent menu
        if (mCurrentMenu == mPlayerCollisionMenu) {
            endSubMenuToParent(mGameplayMenu, mGameplayList);
        } else if (mCurrentMenu == mServerBrowserMenu) {
            endSubMenuToParent(mNetworkMenu, mNetworkList);
        } else if (mCurrentMenu == mModeSelect || mCurrentMenu == mTwistsMenu) {
            endSubMenuToParent(mGameModeSettingsMenu, mGameModeSettingsList);
        } else if (mGamemodeConfigMenu && mCurrentMenu == mGamemodeConfigMenu->mLayout) {
            endSubMenuToParent(mGameModeSettingsMenu, mGameModeSettingsList);
        } else if (mCurrentMenu == mNetworkMenu || mCurrentMenu == mGameplayMenu || 
                   mCurrentMenu == mGameModeSettingsMenu) {
            endSubMenu();
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

void StageSceneStateServerConfig::refreshMenu(CommonVerticalList* list, 
                                                sead::WFixedSafeString<0x200>* options, 
                                                int count) {
    list->initDataNoResetSelected(count);
    list->addStringData(options, "TxtContent");
    list->updateParts();
    activateInput();
}

void StageSceneStateServerConfig::endSubMenu() {
    mCurrentList->deactivate();
    mCurrentMenu->startEnd("End");
    mCurrentList = mMainOptionsList;
    mCurrentMenu = mMainOptions;
    mCurrentMenu->startAppear("Appear");
    al::startHitReaction(mCurrentMenu, "リセット", 0);
    al::setNerve(this, &nrvStageSceneStateServerConfigMainMenu);
}

void StageSceneStateServerConfig::endSubMenuToParent(SimpleLayoutMenu* parentMenu, 
                                                      CommonVerticalList* parentList) {
    mCurrentList->deactivate();
    mCurrentMenu->startEnd("End");
    mCurrentList = parentList;
    mCurrentMenu = parentMenu;
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

// ============================================================================
// Static Getters
// ============================================================================

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

al::MessageSystem* StageSceneStateServerConfig::getMessageSystem() const {
    return mMsgSystem;
}

// ============================================================================
// Nerve Implementations
// ============================================================================

namespace {
    NERVE_IMPL(StageSceneStateServerConfig, MainMenu)
    NERVE_IMPL(StageSceneStateServerConfig, NetworkSettings)
    NERVE_IMPL(StageSceneStateServerConfig, ServerBrowserSelect)
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