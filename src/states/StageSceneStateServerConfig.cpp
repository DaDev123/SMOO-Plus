#include "Scene/StageSceneStateServerConfig.hpp"

#include "al/Library/Layout/LayoutActionFunction.h"
#include "al/Library/LiveActor/ActorInitInfo.h"
#include "al/Library/Nerve/NerveUtil.h"

#include "sead/container/seadSafeArray.h"
#include "sead/prim/seadSafeString.h"

#include "game/Sequence/ChangeStageInfo.h"
#include "game/System/GameDataFile.h"
#include "game/System/GameDataFunction.h"
#include "game/System/GameDataHolderAccessor.h"
#include "game/System/SaveDataAccessFunction.h"
#include "game/Util/StageInputFunction.h"

#include <cstdlib>
#include <cstring>
#include <vector>

#include "BloodMoon/BloodMoonUtils.hpp"
#include "server/Client.hpp"
#include "server/gamemode/GameModeFactory.hpp"
#include "server/gamemode/GameModeManager.hpp"
#include "TwistsConfig.hpp"

// ============================================================================
// Static Configuration Variables
// ============================================================================

bool StageSceneStateServerConfig::sCapCollisionEnabled = false;
bool StageSceneStateServerConfig::sCapBounceEnabled = false;
bool StageSceneStateServerConfig::sPuppetCollisionEnabled = true;
bool StageSceneStateServerConfig::sPuppetBounceEnabled = true;
bool StageSceneStateServerConfig::sCostumeDoorsUnlocked = true;
bool StageSceneStateServerConfig::sLowLatencyEnabled = true;
bool StageSceneStateServerConfig::sSpeedrunModeEnabled = false;
bool StageSceneStateServerConfig::sSpeedrunNonStopEnabled = false;

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
        servers.push_back(ServerBrowser("ERROR: OnlineData/ServerList.txt not found", "", 0));
        return servers;
    }

    char* buffer = reinterpret_cast<char*>(fileData);
    char* savePtr = nullptr;
    char* line = strtok_r(buffer, "\n\r", &savePtr);

    while (line) {
        // Skip whitespace and comments
        while (*line == ' ' || *line == '\t')
            line++;
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

StageSceneStateServerConfig::StageSceneStateServerConfig(const char* name, al::Scene* scene, const al::LayoutInitInfo& initInfo, FooterParts* footerParts,
                                                         GameDataHolder* dataHolder, bool)
    : al::HostStateBase<al::Scene>(name, scene) {
    mFooterParts = footerParts;
    mGameDataHolder = dataHolder;
    mMsgSystem = initInfo.getMessageSystem();
    mInput = new InputSeparator(mHost, true);

    // Load server list
    mServerBrowserServers = loadServersFromFile();
    mServerBrowserCount = mServerBrowserServers.size();

    for (int i = 0; i < menuCount; i++) {
        msgList[i] = new sead::SafeArray<sead::WFixedSafeString<0x200>, maxMsgCount>();
    }

    // Initialize all menus
    initMainMenu(initInfo);
    initNetworkMenu(initInfo);
    initServerBrowserMenu(initInfo);
    initGameplayMenu(initInfo);
    initPlayerCollisionMenu(initInfo);
    initGameModeMenus(initInfo);
    initTwistsMenu(initInfo);
    initMiscMenu(initInfo);
    initSpeedrunConfigMenu(initInfo);

    mCurrentList = optionsList[MENU_MAIN];
    mCurrentMenu = menuList[MENU_MAIN];
}

StageSceneStateServerConfig::~StageSceneStateServerConfig() {
    delete[] mServerBrowserOptions;
    free(menuList);
    free(optionsList);
    free(msgList);
}

// ============================================================================
// Main Menu
// ============================================================================

void StageSceneStateServerConfig::initMainMenu(const al::LayoutInitInfo& initInfo) {
    menuList[MENU_MAIN] = new SimpleLayoutMenu("ServerConfigMenu", "OptionSelect", initInfo, 0, false);
    optionsList[MENU_MAIN] = new CommonVerticalList(menuList[MENU_MAIN], initInfo, true);
    al::setPaneString(menuList[MENU_MAIN], "TxtOption", u"Mod Configuration", 0);
    optionsList[MENU_MAIN]->unkInt1 = 1;
    optionsList[MENU_MAIN]->initDataNoResetSelected(mMainMenuOptionsCount);
    updateMainMenuOptions();
    optionsList[MENU_MAIN]->addStringData(msgList[MENU_MAIN]->mBuffer, "TxtContent");
}

void StageSceneStateServerConfig::updateMainMenuOptions() {
    msgList[MENU_MAIN]->mBuffer[MAIN_NETWORK_SETTINGS].copy(u"Network Settings");
    msgList[MENU_MAIN]->mBuffer[MAIN_GAMEPLAY_SETTINGS].copy(u"Gameplay Settings");
    msgList[MENU_MAIN]->mBuffer[MAIN_GAMEMODE_SETTINGS].copy(u"Game Mode Settings");
    msgList[MENU_MAIN]->mBuffer[MAIN_MISC_SETTINGS].copy(u"Misc Settings");
}

void StageSceneStateServerConfig::exeMainMenu() {
    if (al::isFirstStep(this))
        activateInput();

    handleMenuInput();

    if (rs::isTriggerUiCancel(mHost)) {
        kill();
        SaveDataAccessFunction::startSaveDataWrite(mGameDataHolder);
    }

    if (mIsDecideConfig && mCurrentList->isDecideEnd()) {
        switch (mCurrentList->mCurSelected) {
        case MAIN_NETWORK_SETTINGS:
            al::setNerve(this, &NrvStageSceneStateServerConfig.NetworkSettings);
            break;
        case MAIN_GAMEPLAY_SETTINGS:
            al::setNerve(this, &NrvStageSceneStateServerConfig.GameplaySettings);
            break;
        case MAIN_GAMEMODE_SETTINGS:
            al::setNerve(this, &NrvStageSceneStateServerConfig.GameModeSettings);
            break;
        case MAIN_MISC_SETTINGS:
            al::setNerve(this, &NrvStageSceneStateServerConfig.MiscSettings);
            break;
        }
    }
}

// ============================================================================
// Network Menu
// ============================================================================

void StageSceneStateServerConfig::initNetworkMenu(const al::LayoutInitInfo& initInfo) {
    menuList[MENU_NETWORK] = new SimpleLayoutMenu("NetworkMenu", "OptionSelect", initInfo, 0, false);
    optionsList[MENU_NETWORK] = new CommonVerticalList(menuList[MENU_NETWORK], initInfo, true);
    al::setPaneString(menuList[MENU_NETWORK], "TxtOption", u"Network Settings", 0);
    optionsList[MENU_NETWORK]->unkInt1 = 1;
    optionsList[MENU_NETWORK]->initDataNoResetSelected(mNetworkMenuOptionsCount);
    updateNetworkSettingsOptions();
    optionsList[MENU_NETWORK]->addStringData(msgList[MENU_NETWORK]->mBuffer, "TxtContent");
}

void StageSceneStateServerConfig::updateNetworkSettingsOptions() {
    msgList[MENU_NETWORK]->mBuffer[NETW_SERVERLIST].copy(u"Browse Server List");
    msgList[MENU_NETWORK]->mBuffer[NETW_SERVERIP].copy(u"Custom Server IP");
    msgList[MENU_NETWORK]->mBuffer[NETW_SERVERPORT].copy(u"Custom Server Port");
    msgList[MENU_NETWORK]->mBuffer[NETW_RECONNECT].copy(Client::get()->mIsAllowReconnect ? u"Reconnect to Server" : u"Reconnect to Server (Disabled)");
}

void StageSceneStateServerConfig::exeNetworkSettings() {
    if (al::isFirstStep(this)) {
        mCurrentList = optionsList[MENU_NETWORK];
        mCurrentMenu = menuList[MENU_NETWORK];
        subMenuStart();
    }

    subMenuUpdate();

    if (mIsDecideConfig && mCurrentList->isDecideEnd()) {
        switch (mCurrentList->mCurSelected) {
        case NETW_SERVERLIST:
            al::setNerve(this, &NrvStageSceneStateServerConfig.ServerBrowserSelect);
            break;
        case NETW_SERVERIP:
            al::setNerve(this, &NrvStageSceneStateServerConfig.OpenKeyboardIP);
            break;
        case NETW_SERVERPORT:
            al::setNerve(this, &NrvStageSceneStateServerConfig.OpenKeyboardPort);
            break;
        case NETW_RECONNECT:
            Client::restartConnection();
            updateNetworkSettingsOptions();
            refreshMenu(optionsList[MENU_NETWORK], msgList[MENU_NETWORK]->mBuffer, mNetworkMenuOptionsCount);
            break;
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
        al::setNerve(this, &NrvStageSceneStateServerConfig.NetworkSettings);
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
        al::setNerve(this, &NrvStageSceneStateServerConfig.NetworkSettings);
    }
}

// ============================================================================
// Server Browser Menu
// ============================================================================

void StageSceneStateServerConfig::initServerBrowserMenu(const al::LayoutInitInfo& initInfo) {
    menuList[MENU_SERVERBROWSER] = new SimpleLayoutMenu("ServerBrowserMenu", "OptionSelect", initInfo, 0, false);
    optionsList[MENU_SERVERBROWSER] = new CommonVerticalList(menuList[MENU_SERVERBROWSER], initInfo, true);
    al::setPaneString(menuList[MENU_SERVERBROWSER], "TxtOption", u"Server List (OnlineData/ServerList.txt)", 0);
    optionsList[MENU_SERVERBROWSER]->unkInt1 = 1;
    optionsList[MENU_SERVERBROWSER]->initDataNoResetSelected(mServerBrowserCount);

    mServerBrowserOptions = new sead::WFixedSafeString<0x200>[mServerBrowserCount];
    for (int i = 0; i < mServerBrowserCount; i++) {
        mServerBrowserOptions[i].convertFromMultiByteString(mServerBrowserServers[i].name, strlen(mServerBrowserServers[i].name));
    }
    optionsList[MENU_SERVERBROWSER]->addStringData(mServerBrowserOptions, "TxtContent");
}

void StageSceneStateServerConfig::exeServerBrowserSelect() {
    if (al::isFirstStep(this)) {
        mCurrentList = optionsList[MENU_SERVERBROWSER];
        mCurrentMenu = menuList[MENU_SERVERBROWSER];
        subMenuStart();
    }

    subMenuUpdate();

    if (mIsDecideConfig && mCurrentList->isDecideEnd()) {
        int selected = mCurrentList->mCurSelected;
        if (selected >= 0 && selected < mServerBrowserCount) {
            Client::setServerIP(mServerBrowserServers[selected].ip);
            Client::setServerPort(mServerBrowserServers[selected].port);
            endSubMenuToParent(menuList[MENU_NETWORK], optionsList[MENU_NETWORK]);
        }
    }
}

// ============================================================================
// Gameplay Menu
// ============================================================================

void StageSceneStateServerConfig::initGameplayMenu(const al::LayoutInitInfo& initInfo) {
    menuList[MENU_GAMEPLAY] = new SimpleLayoutMenu("GameplayMenu", "OptionSelect", initInfo, 0, false);
    optionsList[MENU_GAMEPLAY] = new CommonVerticalList(menuList[MENU_GAMEPLAY], initInfo, true);
    al::setPaneString(menuList[MENU_GAMEPLAY], "TxtOption", u"Gameplay Settings", 0);
    optionsList[MENU_GAMEPLAY]->unkInt1 = 1;
    optionsList[MENU_GAMEPLAY]->initDataNoResetSelected(mGameplayMenuOptionsCount);
    updateGameplaySettingsOptions();
    optionsList[MENU_GAMEPLAY]->addStringData(msgList[MENU_GAMEPLAY]->mBuffer, "TxtContent");
}

void StageSceneStateServerConfig::updateGameplaySettingsOptions() {
    msgList[MENU_GAMEPLAY]->mBuffer[GP_PLAYERCOLLISION].copy(u"Player Collision Settings");
    msgList[MENU_GAMEPLAY]->mBuffer[GP_COSTUMEDOORS].copy(sCostumeDoorsUnlocked ? u"Unlock Costume Doors (ON)" : u"Unlock Costume Doors (OFF)");
    msgList[MENU_GAMEPLAY]->mBuffer[GP_LATENCY].copy(sLowLatencyEnabled ? u"Reduce Player Latency (OFF)" : u"Reduce Player Latency (ON)");
    msgList[MENU_GAMEPLAY]->mBuffer[GP_MUSIC].copy(Client::isMusicDisabled() ? u"In-Game Music (OFF)" : u"In-Game Music (ON)");
}

void StageSceneStateServerConfig::exeGameplaySettings() {
    if (al::isFirstStep(this)) {
        mCurrentList = optionsList[MENU_GAMEPLAY];
        mCurrentMenu = menuList[MENU_GAMEPLAY];
        subMenuStart();
    }

    subMenuUpdate();

    if (mIsDecideConfig && mCurrentList->isDecideEnd()) {
        // Toggle settings
        switch (mCurrentList->mCurSelected) {
        case GP_PLAYERCOLLISION:
            al::setNerve(this, &NrvStageSceneStateServerConfig.PlayerCollisionSettings);
            return;
        case GP_COSTUMEDOORS:
            sCostumeDoorsUnlocked = !sCostumeDoorsUnlocked;
            break;
        case GP_LATENCY:
            sLowLatencyEnabled = !sLowLatencyEnabled;
            break;
        case GP_MUSIC:
            Client::toggleMusicDisabled();
            break;
        }

        updateGameplaySettingsOptions();
        refreshMenu(optionsList[MENU_GAMEPLAY], msgList[MENU_GAMEPLAY]->mBuffer, mGameplayMenuOptionsCount);
    }
}

// ============================================================================
// Player Collision Menu
// ============================================================================

void StageSceneStateServerConfig::initPlayerCollisionMenu(const al::LayoutInitInfo& initInfo) {
    menuList[MENU_PLAYERCOLLISION] = new SimpleLayoutMenu("PlayerCollisionMenu", "OptionSelect", initInfo, 0, false);
    optionsList[MENU_PLAYERCOLLISION] = new CommonVerticalList(menuList[MENU_PLAYERCOLLISION], initInfo, true);
    al::setPaneString(menuList[MENU_PLAYERCOLLISION], "TxtOption", u"Player Collision", 0);
    optionsList[MENU_PLAYERCOLLISION]->unkInt1 = 1;
    optionsList[MENU_PLAYERCOLLISION]->initDataNoResetSelected(mPlayerCollisionMenuOptionsCount);
    updatePlayerCollisionOptions();
    optionsList[MENU_PLAYERCOLLISION]->addStringData(msgList[MENU_PLAYERCOLLISION]->mBuffer, "TxtContent");
}

void StageSceneStateServerConfig::updatePlayerCollisionOptions() {
    msgList[MENU_PLAYERCOLLISION]->mBuffer[PC_CAPCOLLISION].copy(sCapCollisionEnabled ? u"Cap Collision (ON)" : u"Cap Collision (OFF)");
    msgList[MENU_PLAYERCOLLISION]->mBuffer[PC_CAPBOUNCE].copy(sCapBounceEnabled ? u"Cap Bouncing (ON)" : u"Cap Bouncing (OFF)");
    msgList[MENU_PLAYERCOLLISION]->mBuffer[PC_PLAYERCOLLISION].copy(sPuppetCollisionEnabled ? u"Player Collision (ON)" : u"Player Collision (OFF)");
    msgList[MENU_PLAYERCOLLISION]->mBuffer[PC_PLAYERBOUNCE].copy(sPuppetBounceEnabled ? u"Player Bouncing (ON)" : u"Player Bouncing (OFF)");
}

void StageSceneStateServerConfig::exePlayerCollisionSettings() {
    if (al::isFirstStep(this)) {
        mCurrentList = optionsList[MENU_PLAYERCOLLISION];
        mCurrentMenu = menuList[MENU_PLAYERCOLLISION];
        subMenuStart();
    }

    subMenuUpdate();

    if (mIsDecideConfig && mCurrentList->isDecideEnd()) {
        switch (mCurrentList->mCurSelected) {
        case PC_CAPCOLLISION:
            sCapCollisionEnabled = !sCapCollisionEnabled;
            break;
        case PC_CAPBOUNCE:
            sCapBounceEnabled = !sCapBounceEnabled;
            break;
        case PC_PLAYERCOLLISION:
            sPuppetCollisionEnabled = !sPuppetCollisionEnabled;
            break;
        case PC_PLAYERBOUNCE:
            sPuppetBounceEnabled = !sPuppetBounceEnabled;
            break;
        }

        updatePlayerCollisionOptions();
        refreshMenu(optionsList[MENU_PLAYERCOLLISION], msgList[MENU_PLAYERCOLLISION]->mBuffer, mPlayerCollisionMenuOptionsCount);
    }
}

// ============================================================================
// Game Mode Menus
// ============================================================================

void StageSceneStateServerConfig::initGameModeMenus(const al::LayoutInitInfo& initInfo) {
    // Game Mode Settings
    menuList[MENU_GAMEMODE] = new SimpleLayoutMenu("GameModeSettingsMenu", "OptionSelect", initInfo, 0, false);
    optionsList[MENU_GAMEMODE] = new CommonVerticalList(menuList[MENU_GAMEMODE], initInfo, true);
    al::setPaneString(menuList[MENU_GAMEMODE], "TxtOption", u"Game Mode", 0);
    optionsList[MENU_GAMEMODE]->unkInt1 = 1;
    optionsList[MENU_GAMEMODE]->initDataNoResetSelected(mGameModeMenuOptionsCount);
    updateGameModeSettingsOptions();
    optionsList[MENU_GAMEMODE]->addStringData(msgList[MENU_GAMEMODE]->mBuffer, "TxtContent");

    // Mode Selection
    menuList[MENU_GAMEMODE_MODESEL] = new SimpleLayoutMenu("GameModeSelectMenu", "OptionSelect", initInfo, 0, false);
    optionsList[MENU_GAMEMODE_MODESEL] = new CommonVerticalList(menuList[MENU_GAMEMODE_MODESEL], initInfo, true);
    al::setPaneString(menuList[MENU_GAMEMODE_MODESEL], "TxtOption", u"Select Game Mode", 0);

    const int modeCount = GameModeFactory::getModeCount();
    optionsList[MENU_GAMEMODE_MODESEL]->initDataNoResetSelected(modeCount);

    auto* modeOptions = new sead::SafeArray<sead::WFixedSafeString<0x200>, modeCount>();
    for (size_t i = 0; i < modeCount; i++) {
        const char* modeName = GameModeFactory::getModeName(i);
        modeOptions->mBuffer[i].convertFromMultiByteString(modeName, strlen(modeName));
    }
    optionsList[MENU_GAMEMODE_MODESEL]->addStringData(modeOptions->mBuffer, "TxtContent");

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

void StageSceneStateServerConfig::updateGameModeSettingsOptions() {
    const char* modeName = GameModeFactory::getModeName(GameModeManager::instance()->getGameMode());
    char text[256];
    snprintf(text, sizeof(text), "Configure %s", modeName);
    msgList[MENU_GAMEMODE]->mBuffer[GM_MODESETTINGS].convertFromMultiByteString(text, strlen(text));

    msgList[MENU_GAMEMODE]->mBuffer[GM_TWISTS].copy(u"Twists & Modifiers");
    msgList[MENU_GAMEMODE]->mBuffer[GM_MODESELECT].copy(u"Change Mode");
}

void StageSceneStateServerConfig::exeGameModeSettings() {
    if (al::isFirstStep(this)) {
        mCurrentList = optionsList[MENU_GAMEMODE];
        mCurrentMenu = menuList[MENU_GAMEMODE];
        subMenuStart();
    }

    subMenuUpdate();

    if (mIsDecideConfig && mCurrentList->isDecideEnd()) {
        switch (mCurrentList->mCurSelected) {
        case GM_MODESETTINGS:
            al::setNerve(this, &NrvStageSceneStateServerConfig.GameModeConfig);
            break;
        case GM_TWISTS:
            al::setNerve(this, &NrvStageSceneStateServerConfig.TwistsSettings);
            break;
        case GM_MODESELECT:
            al::setNerve(this, &NrvStageSceneStateServerConfig.GameModeSelect);
            break;
        }
    }
}

void StageSceneStateServerConfig::exeGameModeConfig() {
    if (al::isFirstStep(this)) {
        int mode = GameModeManager::instance()->getGameMode();
        if (mode < 0 || mode >= mGamemodeConfigMenus.size()) {
            endSubMenuToParent(menuList[MENU_GAMEMODE], optionsList[MENU_GAMEMODE]);
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
        case GameModeConfigMenu::UpdateAction::CLOSE:
            endSubMenu();
            break;
        case GameModeConfigMenu::UpdateAction::REFRESH:
            subMenuRefresh();
            break;
        case GameModeConfigMenu::UpdateAction::NOOP:
            activateInput();
            break;
        }
    }
}

void StageSceneStateServerConfig::exeGameModeSelect() {
    if (al::isFirstStep(this)) {
        mCurrentList = optionsList[MENU_GAMEMODE_MODESEL];
        mCurrentMenu = menuList[MENU_GAMEMODE_MODESEL];
        subMenuStart();
    }

    subMenuUpdate();

    if (mIsDecideConfig && mCurrentList->isDecideEnd()) {
        GameModeManager::instance()->setMode(static_cast<GameMode>(mCurrentList->mCurSelected));

        ChangeStageInfo info =
            ChangeStageInfo(Client::get()->getHolder(), Client::get()->getHolder()->getGameDataFile()->mPlayerStartId.cstr(),
                            GameDataFunction::getCurrentStageName(Client::get()->getHolder()), false, -1, (ChangeStageInfo::SubScenarioType)0);
        Client::get()->getHolder()->changeNextStage(&info, 0);

        updateGameModeSettingsOptions();
        optionsList[MENU_GAMEMODE]->initDataNoResetSelected(mGameModeMenuOptionsCount);
        optionsList[MENU_GAMEMODE]->addStringData(msgList[MENU_GAMEMODE]->mBuffer, "TxtContent");
        optionsList[MENU_GAMEMODE]->updateParts();
        endSubMenuToParent(menuList[MENU_GAMEMODE], optionsList[MENU_GAMEMODE]);
    }
}

// ============================================================================
// Twists Menu
// ============================================================================

void StageSceneStateServerConfig::initTwistsMenu(const al::LayoutInitInfo& initInfo) {
    menuList[MENU_TWISTS] = new SimpleLayoutMenu("TwistsMenu", "OptionSelect", initInfo, 0, false);
    optionsList[MENU_TWISTS] = new CommonVerticalList(menuList[MENU_TWISTS], initInfo, true);
    al::setPaneString(menuList[MENU_TWISTS], "TxtOption", u"Twists & Modifiers", 0);
    optionsList[MENU_TWISTS]->unkInt1 = 1;
    optionsList[MENU_TWISTS]->initDataNoResetSelected(mTwistsMenuOptionsCount);
    updateTwistsOptions();
    optionsList[MENU_TWISTS]->addStringData(msgList[MENU_TWISTS]->mBuffer, "TxtContent");
}

void StageSceneStateServerConfig::updateTwistsOptions() {
    msgList[MENU_TWISTS]->mBuffer[TW_DISABLECAP].copy(TwistsConfig::isCappyDisableEnabled() ? u"Disable Cappy (OFF)" : u"Disable Cappy (ON)");
    msgList[MENU_TWISTS]->mBuffer[TW_ICEPHYSICS].copy(TwistsConfig::isIcePhysicsEnabled() ? u"Ice Physics (ON)" : u"Ice Physics (OFF)");
    msgList[MENU_TWISTS]->mBuffer[TW_MORESOON].copy(u"More twists coming soon...");
}

void StageSceneStateServerConfig::exeTwistsSettings() {
    if (al::isFirstStep(this)) {
        mCurrentList = optionsList[MENU_TWISTS];
        mCurrentMenu = menuList[MENU_TWISTS];
        subMenuStart();
    }

    subMenuUpdate();

    if (mIsDecideConfig && mCurrentList->isDecideEnd()) {
        switch (mCurrentList->mCurSelected) {
        case TW_DISABLECAP:
            TwistsConfig::toggleCappyDisable();
            break;
        case TW_ICEPHYSICS:
            TwistsConfig::toggleIcePhysics();
            break;
        }

        updateTwistsOptions();
        refreshMenu(optionsList[MENU_TWISTS], msgList[MENU_TWISTS]->mBuffer, mTwistsMenuOptionsCount);
        mIsDecideConfig = false;
    }
}

// ============================================================================
// Misc Menu
// ============================================================================

void StageSceneStateServerConfig::initMiscMenu(const al::LayoutInitInfo& initInfo) {
    menuList[MENU_MISC] = new SimpleLayoutMenu("MiscMenu", "OptionSelect", initInfo, 0, false);
    optionsList[MENU_MISC] = new CommonVerticalList(menuList[MENU_MISC], initInfo, true);
    al::setPaneString(menuList[MENU_MISC], "TxtOption", u"Misc Settings", 0);
    optionsList[MENU_MISC]->unkInt1 = 1;
    optionsList[MENU_MISC]->initDataNoResetSelected(mMiscMenuOptionsCount);
    updateMiscOptions();
    optionsList[MENU_MISC]->addStringData(msgList[MENU_MISC]->mBuffer, "TxtContent");
}

void StageSceneStateServerConfig::updateMiscOptions() {
    msgList[MENU_MISC]->mBuffer[MISC_SPEEDRUN_MODE].copy(sSpeedrunModeEnabled ? u"Speedrun Mode (ON)" : u"Speedrun Mode (OFF)");
    msgList[MENU_MISC]->mBuffer[MISC_SPEEDRUN_CONFIG].copy(u"Speedrun Config");
}

void StageSceneStateServerConfig::exeMiscSettings() {
    if (al::isFirstStep(this)) {
        mCurrentList = optionsList[MENU_MISC];
        mCurrentMenu = menuList[MENU_MISC];
        subMenuStart();
    }

    subMenuUpdate();

    if (mIsDecideConfig && mCurrentList->isDecideEnd()) {
        switch (mCurrentList->mCurSelected) {
        case MISC_SPEEDRUN_MODE:
            sSpeedrunModeEnabled = !sSpeedrunModeEnabled;
            if (sSpeedrunModeEnabled) {
                GameModeBase* mode = GameModeManager::instance()->getMode<GameModeBase>();
                if (mode && mode->isModeActive()) {
                    mode->end();
                }
            }

            updateMiscOptions();
            refreshMenu(optionsList[MENU_MISC], msgList[MENU_MISC]->mBuffer, mMiscMenuOptionsCount);
            break;

        case MISC_SPEEDRUN_CONFIG:
            al::setNerve(this, &NrvStageSceneStateServerConfig.SpeedrunConfig);
            break;
        }
    }
}

// ============================================================================
// Speedrun Config Menu
// ============================================================================

void StageSceneStateServerConfig::initSpeedrunConfigMenu(const al::LayoutInitInfo& initInfo) {
    menuList[MENU_SPEEDRUN_CONFIG] = new SimpleLayoutMenu("SpeedrunConfigMenu", "OptionSelect", initInfo, 0, false);
    optionsList[MENU_SPEEDRUN_CONFIG] = new CommonVerticalList(menuList[MENU_SPEEDRUN_CONFIG], initInfo, true);
    al::setPaneString(menuList[MENU_SPEEDRUN_CONFIG], "TxtOption", u"Speedrun Config", 0);
    optionsList[MENU_SPEEDRUN_CONFIG]->unkInt1 = 1;
    optionsList[MENU_SPEEDRUN_CONFIG]->initDataNoResetSelected(mSpeedrunConfigOptionsCount);
    updateSpeedrunConfigOptions();
    optionsList[MENU_SPEEDRUN_CONFIG]->addStringData(msgList[MENU_SPEEDRUN_CONFIG]->mBuffer, "TxtContent");
}

void StageSceneStateServerConfig::updateSpeedrunConfigOptions() {
    msgList[MENU_SPEEDRUN_CONFIG]->mBuffer[SPEEDRUN_NONSTOP].copy(sSpeedrunNonStopEnabled ? u"Non-Stop (WIP)" : u"Non-Stop (WIP)");
}

void StageSceneStateServerConfig::exeSpeedrunConfig() {
    if (al::isFirstStep(this)) {
        mCurrentList = optionsList[MENU_SPEEDRUN_CONFIG];
        mCurrentMenu = menuList[MENU_SPEEDRUN_CONFIG];
        subMenuStart();
    }

    subMenuUpdate();

    if (mIsDecideConfig && mCurrentList->isDecideEnd()) {
        switch (mCurrentList->mCurSelected) {
        case SPEEDRUN_NONSTOP:
            sSpeedrunNonStopEnabled = !sSpeedrunNonStopEnabled;
            // TODO: Implement non-stop
            break;
        }

        updateSpeedrunConfigOptions();
        refreshMenu(optionsList[MENU_SPEEDRUN_CONFIG], msgList[MENU_SPEEDRUN_CONFIG]->mBuffer, mSpeedrunConfigOptionsCount);
    }
}

// ============================================================================
// Lifecycle Methods
// ============================================================================

void StageSceneStateServerConfig::init() {
    initNerve(&NrvStageSceneStateServerConfig.MainMenu, 0);
}

void StageSceneStateServerConfig::appear() {
    mCurrentMenu->startAppear("Appear");
    al::NerveStateBase::appear();
}

void StageSceneStateServerConfig::kill() {
    if (Client::hasServerChanged()) {
        if (Client::get()->mIsAllowReconnect)
            Client::restartConnection();
        Client::showUIMessage(Client::get()->mIsAllowReconnect ? u"Reconnecting..." : u"Server changed. Please restart the game.");
        for (int i = 0; i < 180; i++)
            nn::os::YieldThread();
        Client::hideUIMessage();
    }
    mCurrentMenu->startEnd("End");
    al::NerveStateBase::kill();
}

// ============================================================================
// Option Update Methods
// ============================================================================

void StageSceneStateServerConfig::exeSaveData() {
    if (al::isFirstStep(this)) {
        SaveDataAccessFunction::startSaveDataWrite(mGameDataHolder);
    }

    if (SaveDataAccessFunction::updateSaveDataAccess(mGameDataHolder, false)) {
        al::startHitReaction(mCurrentMenu, "リセット", 0);

        // Return to Network Settings after saving from keyboard input
        mCurrentList->activate();
        mCurrentList->appearCursor();
        al::setNerve(this, &NrvStageSceneStateServerConfig.NetworkSettings);
    }
}

// ============================================================================
// Helper Methods
// ============================================================================

void StageSceneStateServerConfig::handleMenuInput() {
    mInput->update();
    mCurrentList->update();
    if (mInput->isTriggerUiUp())
        mCurrentList->up();
    if (mInput->isTriggerUiDown())
        mCurrentList->down();
    if (rs::isTriggerUiDecide(mHost))
        deactivateInput();
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
        if (mCurrentMenu == menuList[MENU_PLAYERCOLLISION]) {
            endSubMenuToParent(menuList[MENU_GAMEPLAY], optionsList[MENU_GAMEPLAY]);
        } else if (mCurrentMenu == menuList[MENU_SERVERBROWSER]) {
            endSubMenuToParent(menuList[MENU_NETWORK], optionsList[MENU_NETWORK]);
        } else if (mCurrentMenu == menuList[MENU_GAMEMODE_MODESEL] || mCurrentMenu == menuList[MENU_TWISTS]) {
            endSubMenuToParent(menuList[MENU_GAMEMODE], optionsList[MENU_GAMEMODE]);
        } else if (mCurrentMenu == menuList[MENU_SPEEDRUN_CONFIG]) {
            endSubMenuToParent(menuList[MENU_MISC], optionsList[MENU_MISC]);
        } else if (mGamemodeConfigMenu && mCurrentMenu == mGamemodeConfigMenu->mLayout) {
            endSubMenuToParent(menuList[MENU_GAMEMODE], optionsList[MENU_GAMEMODE]);
        } else {
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

void StageSceneStateServerConfig::refreshMenu(CommonVerticalList* list, sead::WFixedSafeString<0x200>* options, int count) {
    list->initDataNoResetSelected(count);
    list->addStringData(options, "TxtContent");
    list->updateParts();
    activateInput();
}

void StageSceneStateServerConfig::endSubMenu() {
    mCurrentList->deactivate();
    mCurrentMenu->startEnd("End");
    mCurrentList = optionsList[MENU_MAIN];
    mCurrentMenu = menuList[MENU_MAIN];
    mCurrentMenu->startAppear("Appear");
    al::startHitReaction(mCurrentMenu, "リセット", 0);
    al::setNerve(this, &NrvStageSceneStateServerConfig.MainMenu);
}

void StageSceneStateServerConfig::endSubMenuToParent(SimpleLayoutMenu* parentMenu, CommonVerticalList* parentList) {
    mCurrentList->deactivate();
    mCurrentMenu->startEnd("End");
    mCurrentList = parentList;
    mCurrentMenu = parentMenu;
    activateInput();
    mIsDecideConfig = false;

    // Set appropriate nerve based on parent menu
    if (parentMenu == menuList[MENU_GAMEPLAY]) {
        al::setNerve(this, &NrvStageSceneStateServerConfig.GameplaySettings);
    } else if (parentMenu == menuList[MENU_NETWORK]) {
        al::setNerve(this, &NrvStageSceneStateServerConfig.NetworkSettings);
    } else if (parentMenu == menuList[MENU_GAMEMODE]) {
        al::setNerve(this, &NrvStageSceneStateServerConfig.GameModeSettings);
    } else if (parentMenu == menuList[MENU_MISC]) {
        al::setNerve(this, &NrvStageSceneStateServerConfig.MiscSettings);
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
