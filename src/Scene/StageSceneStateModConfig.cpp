#include "Scene/StageSceneStateModConfig.hpp"

#include "sead/container/seadSafeArray.h"
#include "sead/prim/seadSafeString.h"

#include "al/Library/Layout/LayoutActionFunction.h"
#include "al/Library/Layout/LayoutActorUtil.h"
#include "al/Library/Layout/LayoutInitInfo.h"
#include "al/Library/LiveActor/ActorInitInfo.h"
#include "al/Library/Memory/HeapUtil.h"
#include "al/Library/Nerve/NerveUtil.h"
#include "al/Library/Play/Layout/RollParts.h"

#include "game/Layout/CommonVerticalList.h"
#include "game/Layout/FooterParts.h"
#include "game/Layout/SimpleLayoutMenu.h"
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
#include "heap/seadHeapMgr.h"
#include "Scene/Twists/TwistsConfig.hpp"
#include "server/Client.hpp"
#include "server/gamemode/GameModeConfigMenu.hpp"
#include "server/gamemode/GameModeFactory.hpp"
#include "server/gamemode/GameModeManager.hpp"

// ============================================================================
// Static Configuration Variables
// ============================================================================

bool StageSceneStateModConfig::sCapCollisionEnabled = false;
bool StageSceneStateModConfig::sCapBounceEnabled = false;
bool StageSceneStateModConfig::sPuppetCollisionEnabled = true;
bool StageSceneStateModConfig::sPuppetBounceEnabled = true;
bool StageSceneStateModConfig::sCostumeDoorsUnlocked = true;
bool StageSceneStateModConfig::sLowLatencyEnabled = true;
bool StageSceneStateModConfig::sSpeedrunModeEnabled = false;
bool StageSceneStateModConfig::sSpeedrunNonStopEnabled = false;

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
// Helper: does the current menu have roll parts on the selected item?
// ============================================================================

bool StageSceneStateModConfig::currentMenuHasRollParts() const {
    if (mCurrentMenu == menuList[MENU_GAMEPLAY])
        return true;
    if (mGamemodeConfigMenu && mCurrentMenu == mGamemodeConfigMenu->mMenu)
        return true;
    if (mCurrentMenu == menuList[MENU_SPEEDRUN_CONFIG]) {
        // Only the first item (SPEEDRUN_NONSTOP) is a check; item at index 2 is roll
        // Guard: only allow roll input when the selected row actually is a roll part
        return (mCurrentList->mCurSelected == 2);
    }
    return false;
}

// ============================================================================
// Constructor
// ============================================================================

StageSceneStateModConfig::StageSceneStateModConfig(const char* name, al::Scene* scene, const al::LayoutInitInfo& initInfo, FooterParts* footerParts,
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
    initGameModeMenus(initInfo);
    initTwistsMenu(initInfo);
    initMiscMenu(initInfo);
    initSpeedrunConfigMenu(initInfo);

    mCurrentList = optionsList[MENU_MAIN];
    mCurrentMenu = menuList[MENU_MAIN];
}

StageSceneStateModConfig::~StageSceneStateModConfig() {
    delete[] mServerBrowserOptions;
    free(menuList);
    free(optionsList);
    free(msgList);
}

// ============================================================================
// Main Menu
// ============================================================================

void StageSceneStateModConfig::initMainMenu(const al::LayoutInitInfo& initInfo) {
    menuList[MENU_MAIN] = new SimpleLayoutMenu("ModConfigMenu", "OptionModCheck", initInfo, 0, false);
    optionsList[MENU_MAIN] = new CommonVerticalList(menuList[MENU_MAIN], initInfo, true);
    al::setPaneString(menuList[MENU_MAIN], "TxtOption", u"Mod Configuration", 0);
    optionsList[MENU_MAIN]->initDataNoResetSelected(mMainMenuOptionsCount);
    updateMainMenuOptions();
    optionsList[MENU_MAIN]->addStringData(msgList[MENU_MAIN]->mBuffer, "TxtContent");

    for (int i = 0; i < mMainMenuOptionsCount; i++) {
        setMenuItemBase(optionsList[MENU_MAIN]->mListPartsArr[i + 1]);
    }
}

void StageSceneStateModConfig::updateMainMenuOptions() {
    msgList[MENU_MAIN]->mBuffer[MAIN_NETWORK_SETTINGS].copy(u"Network Settings");
    msgList[MENU_MAIN]->mBuffer[MAIN_GAMEPLAY_SETTINGS].copy(u"Gameplay Settings");
    msgList[MENU_MAIN]->mBuffer[MAIN_GAMEMODE_SETTINGS].copy(u"Game Mode Settings");
    msgList[MENU_MAIN]->mBuffer[MAIN_MISC_SETTINGS].copy(u"Misc Settings");
}

void StageSceneStateModConfig::exeMainMenu() {
    // Always set current list/menu on first step so mCurrentList is never null
    // when handleMenuInput() is called, regardless of how this nerve was entered.
    if (al::isFirstStep(this)) {
        mCurrentList = optionsList[MENU_MAIN];
        mCurrentMenu = menuList[MENU_MAIN];
        activateInput();
    }

    handleMenuInput();

    if (rs::isTriggerUiCancel(mHost)) {
        kill();
        SaveDataAccessFunction::startSaveDataWrite(mGameDataHolder);
    }

    if (mIsDecideConfig && mCurrentList->isDecideEnd()) {
        switch (mCurrentList->mCurSelected) {
        case MAIN_NETWORK_SETTINGS:
            al::setNerve(this, &NrvStageSceneStateModConfig.NetworkSettings);
            break;
        case MAIN_GAMEPLAY_SETTINGS:
            al::setNerve(this, &NrvStageSceneStateModConfig.GameplaySettings);
            break;
        case MAIN_GAMEMODE_SETTINGS:
            if (!sSpeedrunModeEnabled) {
                al::setNerve(this, &NrvStageSceneStateModConfig.GameModeSettings);
            } else {
                al::setNerve(this, &NrvStageSceneStateModConfig.MainMenu);
                Client::showUIMessage(u"You cannot use Gamemodes in Speedrun Mode");
                Client::hideUIMessage();
            }
            break;
        case MAIN_MISC_SETTINGS:
            al::setNerve(this, &NrvStageSceneStateModConfig.MiscSettings);
            break;
        }
    }
}

// ============================================================================
// Network Menu
// ============================================================================

void StageSceneStateModConfig::initNetworkMenu(const al::LayoutInitInfo& initInfo) {
    menuList[MENU_NETWORK] = new SimpleLayoutMenu("NetworkMenu", "OptionModCheck", initInfo, 0, false);
    optionsList[MENU_NETWORK] = new CommonVerticalList(menuList[MENU_NETWORK], initInfo, true);
    al::setPaneString(menuList[MENU_NETWORK], "TxtOption", u"Network Settings", 0);
    optionsList[MENU_NETWORK]->initDataNoResetSelected(mNetworkMenuOptionsCount);
    updateNetworkSettingsOptions();
    optionsList[MENU_NETWORK]->addStringData(msgList[MENU_NETWORK]->mBuffer, "TxtContent");

    for (int i = 0; i < mNetworkMenuOptionsCount; i++) {
        setMenuItemBase(optionsList[MENU_NETWORK]->mListPartsArr[i + 1]);
    }
}

void StageSceneStateModConfig::updateNetworkSettingsOptions() {
    msgList[MENU_NETWORK]->mBuffer[NETW_SERVERLIST].copy(u"Browse Server List");
    msgList[MENU_NETWORK]->mBuffer[NETW_SERVERIP].copy(u"Custom Server IP");
    msgList[MENU_NETWORK]->mBuffer[NETW_SERVERPORT].copy(u"Custom Server Port");
    msgList[MENU_NETWORK]->mBuffer[NETW_RECONNECT].copy(Client::get()->mIsAllowReconnect ? u"Reconnect to Server" : u"Reconnect to Server (Disabled)");
}

void StageSceneStateModConfig::exeNetworkSettings() {
    if (al::isFirstStep(this)) {
        mCurrentList = optionsList[MENU_NETWORK];
        mCurrentMenu = menuList[MENU_NETWORK];
        subMenuStart();
    }

    subMenuUpdate();

    if (mIsDecideConfig && mCurrentList->isDecideEnd()) {
        switch (mCurrentList->mCurSelected) {
        case NETW_SERVERLIST:
            al::setNerve(this, &NrvStageSceneStateModConfig.ServerBrowserSelect);
            break;
        case NETW_SERVERIP:
            al::setNerve(this, &NrvStageSceneStateModConfig.OpenKeyboardIP);
            break;
        case NETW_SERVERPORT:
            al::setNerve(this, &NrvStageSceneStateModConfig.OpenKeyboardPort);
            break;
        case NETW_RECONNECT:
            Client::restartConnection();
            updateNetworkSettingsOptions();
            activateInput();
            break;
        }
    }
}

void StageSceneStateModConfig::exeOpenKeyboardIP() {
    if (al::isFirstStep(this)) {
        mCurrentList->deactivate();
        Client::getKeyboard()->setHeaderText(u"Enter Server IP Address");
        Client::getKeyboard()->setSubText(u"");
        bool isSave = Client::openKeyboardIP();

        if (isSave) {
            SaveDataAccessFunction::startSaveDataWrite(mGameDataHolder);
        }

        al::startHitReaction(mCurrentMenu, "リセット", 0);
        mCurrentList->activate();
        mCurrentList->appearCursor();
        al::setNerve(this, &NrvStageSceneStateModConfig.NetworkSettings);
    }
}

void StageSceneStateModConfig::exeOpenKeyboardPort() {
    if (al::isFirstStep(this)) {
        mCurrentList->deactivate();
        Client::getKeyboard()->setHeaderText(u"Enter Server Port");
        Client::getKeyboard()->setSubText(u"");
        bool isSave = Client::openKeyboardPort();

        if (isSave) {
            SaveDataAccessFunction::startSaveDataWrite(mGameDataHolder);
        }

        al::startHitReaction(mCurrentMenu, "リセット", 0);
        mCurrentList->activate();
        mCurrentList->appearCursor();
        al::setNerve(this, &NrvStageSceneStateModConfig.NetworkSettings);
    }
}

// ============================================================================
// Server Browser Menu
// ============================================================================

void StageSceneStateModConfig::initServerBrowserMenu(const al::LayoutInitInfo& initInfo) {
    menuList[MENU_SERVERBROWSER] = new SimpleLayoutMenu("ServerBrowserMenu", "OptionModCheck", initInfo, 0, false);
    optionsList[MENU_SERVERBROWSER] = new CommonVerticalList(menuList[MENU_SERVERBROWSER], initInfo, true);
    al::setPaneString(menuList[MENU_SERVERBROWSER], "TxtOption", u"Server List (OnlineData/ServerList.txt)", 0);
    optionsList[MENU_SERVERBROWSER]->initDataNoResetSelected(mServerBrowserCount);

    mServerBrowserOptions = new sead::WFixedSafeString<0x200>[mServerBrowserCount];
    for (int i = 0; i < mServerBrowserCount; i++) {
        mServerBrowserOptions[i].convertFromMultiByteString(mServerBrowserServers[i].name, strlen(mServerBrowserServers[i].name));
    }
    optionsList[MENU_SERVERBROWSER]->addStringData(mServerBrowserOptions, "TxtContent");

    for (int i = 0; i < mServerBrowserCount; i++) {
        setMenuItemBase(optionsList[MENU_SERVERBROWSER]->mListPartsArr[i + 1]);
    }
}

void StageSceneStateModConfig::exeServerBrowserSelect() {
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

void StageSceneStateModConfig::initGameplayMenu(const al::LayoutInitInfo& initInfo) {
    menuList[MENU_GAMEPLAY] = new SimpleLayoutMenu("GameplayMenu", "OptionModCheck", initInfo, 0, false);
    optionsList[MENU_GAMEPLAY] = new CommonVerticalList(menuList[MENU_GAMEPLAY], initInfo, true);
    al::setPaneString(menuList[MENU_GAMEPLAY], "TxtOption", u"Gameplay Settings", 0);
    optionsList[MENU_GAMEPLAY]->initDataNoResetSelected(mGameplayMenuOptionsCount);

    setMenuItemRoll(optionsList[MENU_GAMEPLAY]->mListPartsArr[GP_PLAYERCOL + 1]);
    setMenuItemRoll(optionsList[MENU_GAMEPLAY]->mListPartsArr[GP_CAPCOL + 1]);
    setMenuItemCheck(optionsList[MENU_GAMEPLAY]->mListPartsArr[GP_COSTUMEDOORS + 1]);
    setMenuItemCheck(optionsList[MENU_GAMEPLAY]->mListPartsArr[GP_LATENCY + 1]);
    setMenuItemCheck(optionsList[MENU_GAMEPLAY]->mListPartsArr[GP_MUSIC + 1]);

    sead::ScopedCurrentHeapSetter setter(al::getSceneHeap());
    optionsList[MENU_GAMEPLAY]->startLoopActionAll("Loop", "Loop");
    RollPartsData* dataColPlayer = new RollPartsData(
        4, new const char16_t* [] { u"Off", u"Collision", u"Bounce", u"Collision + Bounce" }, (sPuppetCollisionEnabled + (sPuppetBounceEnabled << 1)), false);
    RollPartsData* dataColCap = new RollPartsData(
        4, new const char16_t* [] { u"Off", u"Collision", u"Bounce", u"Collision + Bounce" }, (sCapCollisionEnabled + (sCapBounceEnabled << 1)), false);
    RollPartsData* dataEmpty = new RollPartsData(0, new const char16_t* [] { u"" });
    optionsList[MENU_GAMEPLAY]->setRollPartsData(new RollPartsData[]{*dataColPlayer, *dataColCap, *dataEmpty, *dataEmpty, *dataEmpty});

    optionsList[MENU_GAMEPLAY]->addStringData(msgList[MENU_GAMEPLAY]->mBuffer, "TxtContent");
    updateGameplaySettingsOptions();
}

void StageSceneStateModConfig::updateGameplaySettingsOptions() {
    msgList[MENU_GAMEPLAY]->mBuffer[GP_PLAYERCOL].copy(u"Player Interaction");
    msgList[MENU_GAMEPLAY]->mBuffer[GP_CAPCOL].copy(u"Cappy Interaction");
    msgList[MENU_GAMEPLAY]->mBuffer[GP_COSTUMEDOORS].copy(u"Unlock Costume Doors");
    al::startAction(optionsList[MENU_GAMEPLAY]->mListPartsArr[GP_COSTUMEDOORS + 1], sCostumeDoorsUnlocked ? "On" : "Off", "State");

    msgList[MENU_GAMEPLAY]->mBuffer[GP_LATENCY].copy(u"Reduce Player Latency");
    al::startAction(optionsList[MENU_GAMEPLAY]->mListPartsArr[GP_LATENCY + 1], sLowLatencyEnabled ? "On" : "Off", "State");

    msgList[MENU_GAMEPLAY]->mBuffer[GP_MUSIC].copy(u"In-Game Music");
    al::startAction(optionsList[MENU_GAMEPLAY]->mListPartsArr[GP_MUSIC + 1], Client::isMusicDisabled() ? "Off" : "On", "State");
}

void StageSceneStateModConfig::exeGameplaySettings() {
    if (al::isFirstStep(this)) {
        mCurrentList = optionsList[MENU_GAMEPLAY];
        mCurrentMenu = menuList[MENU_GAMEPLAY];
        subMenuStart();
    }

    subMenuUpdate();

    if (mIsDecideConfig && mCurrentList->isDecideEnd()) {
        switch (mCurrentList->mCurSelected) {
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
        activateInput();
    }
}

// ============================================================================
// Game Mode Menus
// ============================================================================

void StageSceneStateModConfig::initGameModeMenus(const al::LayoutInitInfo& initInfo) {
    // Game Mode Settings
    menuList[MENU_GAMEMODE] = new SimpleLayoutMenu("GameModeSettingsMenu", "OptionModCheck", initInfo, 0, false);
    optionsList[MENU_GAMEMODE] = new CommonVerticalList(menuList[MENU_GAMEMODE], initInfo, true);
    al::setPaneString(menuList[MENU_GAMEMODE], "TxtOption", u"Game Mode", 0);
    optionsList[MENU_GAMEMODE]->initDataNoResetSelected(mGameModeMenuOptionsCount);

    for (int i = 0; i < mGameModeMenuOptionsCount; i++) {
        setMenuItemBase(optionsList[MENU_GAMEMODE]->mListPartsArr[i + 1]);
    }

    updateGameModeSettingsOptions();
    optionsList[MENU_GAMEMODE]->addStringData(msgList[MENU_GAMEMODE]->mBuffer, "TxtContent");

    // Mode Selection
    menuList[MENU_GAMEMODE_MODESEL] = new SimpleLayoutMenu("GameModeSelectMenu", "OptionModCheck", initInfo, 0, false);
    optionsList[MENU_GAMEMODE_MODESEL] = new CommonVerticalList(menuList[MENU_GAMEMODE_MODESEL], initInfo, true);
    al::setPaneString(menuList[MENU_GAMEMODE_MODESEL], "TxtOption", u"Select Game Mode", 0);

    const int modeCount = GameModeFactory::getModeCount();
    optionsList[MENU_GAMEMODE_MODESEL]->initDataNoResetSelected(modeCount);

    for (int i = 0; i < modeCount; i++) {
        setMenuItemBase(optionsList[MENU_GAMEMODE_MODESEL]->mListPartsArr[i + 1]);
    }

    auto* modeOptions = new sead::SafeArray<sead::WFixedSafeString<0x200>, modeCount>();
    for (size_t i = 0; i < modeCount; i++) {
        const char* modeName = GameModeFactory::getModeName(i);
        modeOptions->mBuffer[i].convertFromMultiByteString(modeName, strlen(modeName));
    }
    optionsList[MENU_GAMEMODE_MODESEL]->addStringData(modeOptions->mBuffer, "TxtContent");

    // Mode Config
    GameModeConfigMenuFactory factory("GameModeConfigFactory");
    for (int mode = 0; mode < factory.getMenuCount(); mode++) {
        const char* name = factory.getMenuName(mode);
        mGamemodeConfigMenus[mode] = factory.getCreator(name)(name);
        GameModeConfigMenu& entry = *mGamemodeConfigMenus[mode];
        entry.mMenu = new (al::getSceneHeap()) SimpleLayoutMenu("GameModeConfigMenu", "OptionModCheck", initInfo, 0, false);
        entry.mList = new (al::getSceneHeap()) CommonVerticalList(entry.mMenu, initInfo, true);

        al::setPaneString(entry.mMenu, "TxtOption", u"Mode Configuration", 0);

        entry.mList->initDataNoResetSelected(entry.getMenuSize());
        entry.initMenu();
        entry.mList->addStringData(entry.getStringData(), "TxtContent");
    }
}

void StageSceneStateModConfig::updateGameModeSettingsOptions() {
    const char* modeName = GameModeFactory::getModeName(GameModeManager::instance()->getGameMode());
    char text[256];
    snprintf(text, sizeof(text), "Configure %s", modeName);
    msgList[MENU_GAMEMODE]->mBuffer[GM_MODESETTINGS].convertFromMultiByteString(text, strlen(text));

    msgList[MENU_GAMEMODE]->mBuffer[GM_TWISTS].copy(u"Twists & Modifiers");
    msgList[MENU_GAMEMODE]->mBuffer[GM_MODESELECT].copy(u"Change Mode");
}

void StageSceneStateModConfig::exeGameModeSettings() {
    if (mShouldHideMessage) {
        mMessageHideTimer++;
        if (mMessageHideTimer >= 60) {
            Client::hideUIMessage();
            mShouldHideMessage = false;
            mMessageHideTimer = 0;
        }
    }

    if (al::isFirstStep(this)) {
        mCurrentList = optionsList[MENU_GAMEMODE];
        mCurrentMenu = menuList[MENU_GAMEMODE];
        subMenuStart();
    }

    subMenuUpdate();

    if (mIsDecideConfig && mCurrentList->isDecideEnd()) {
        switch (mCurrentList->mCurSelected) {
        case GM_MODESETTINGS:
            al::setNerve(this, &NrvStageSceneStateModConfig.GameModeConfig);
            break;
        case GM_TWISTS:
            al::setNerve(this, &NrvStageSceneStateModConfig.TwistsSettings);
            break;
        case GM_MODESELECT:
            al::setNerve(this, &NrvStageSceneStateModConfig.GameModeSelect);
            break;
        }
    }
}

void StageSceneStateModConfig::exeGameModeConfig() {
    if (al::isFirstStep(this)) {
        int mode = GameModeManager::instance()->getGameMode();
        if (mode < 0 || mode >= (int)mGamemodeConfigMenus.size()) {
            endSubMenuToParent(menuList[MENU_GAMEMODE], optionsList[MENU_GAMEMODE]);
            return;
        }

        mGamemodeConfigMenu = mGamemodeConfigMenus[mode];
        mCurrentList = mGamemodeConfigMenu->mList;
        mCurrentMenu = mGamemodeConfigMenu->mMenu;
        subMenuStart();
    }

    subMenuUpdate();

    if (mIsDecideConfig && mCurrentList->isDecideEnd() && mGamemodeConfigMenu) {
        auto action = mGamemodeConfigMenu->updateMenu(mCurrentList->mCurSelected);
        switch (action) {
        case GameModeConfigMenu::UpdateAction::CLOSE:
            endSubMenu();
            break;
        case GameModeConfigMenu::UpdateAction::REFRESH:
        case GameModeConfigMenu::UpdateAction::NOOP:
            activateInput();
            break;
        }
    }
}

void StageSceneStateModConfig::exeGameModeSelect() {
    if (al::isFirstStep(this)) {
        mCurrentList = optionsList[MENU_GAMEMODE_MODESEL];
        mCurrentMenu = menuList[MENU_GAMEMODE_MODESEL];
        subMenuStart();
    }

    subMenuUpdate();

    if (mIsDecideConfig && mCurrentList->isDecideEnd()) {
        GameMode selectedMode = static_cast<GameMode>(mCurrentList->mCurSelected);

        GameModeManager::instance()->setMode(selectedMode);

        ChangeStageInfo info =
            ChangeStageInfo(Client::get()->getHolder(), Client::get()->getHolder()->getGameDataFile()->getPlayerStartId().cstr(),
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

void StageSceneStateModConfig::initTwistsMenu(const al::LayoutInitInfo& initInfo) {
    menuList[MENU_TWISTS] = new SimpleLayoutMenu("TwistsMenu", "OptionModCheck", initInfo, 0, false);
    optionsList[MENU_TWISTS] = new CommonVerticalList(menuList[MENU_TWISTS], initInfo, true);
    al::setPaneString(menuList[MENU_TWISTS], "TxtOption", u"Twists & Modifiers", 0);

    optionsList[MENU_TWISTS]->initDataNoResetSelected(mTwistsMenuOptionsCount);

    setMenuItemCheck(optionsList[MENU_TWISTS]->mListPartsArr[TW_DISABLECAP + 1]);
    setMenuItemCheck(optionsList[MENU_TWISTS]->mListPartsArr[TW_ICEPHYSICS + 1]);
    setMenuItemCheck(optionsList[MENU_TWISTS]->mListPartsArr[TW_SMALLMARIO + 1]);
    setMenuItemCheck(optionsList[MENU_TWISTS]->mListPartsArr[TW_DARKNESS + 1]);
    setMenuItemCheck(optionsList[MENU_TWISTS]->mListPartsArr[TW_TIMEWARP + 1]);
    setMenuItemCheck(optionsList[MENU_TWISTS]->mListPartsArr[TW_TWOD + 1]);
    setMenuItemCheck(optionsList[MENU_TWISTS]->mListPartsArr[TW_FLUDD + 1]);
    setMenuItemCheck(optionsList[MENU_TWISTS]->mListPartsArr[TW_MOONGRAVITY + 1]);

    optionsList[MENU_TWISTS]->addStringData(msgList[MENU_TWISTS]->mBuffer, "TxtContent");
    updateTwistsOptions();
}

void StageSceneStateModConfig::updateTwistsOptions() {
    msgList[MENU_TWISTS]->mBuffer[TW_DISABLECAP].copy(u"Disable Cappy");
    al::startAction(optionsList[MENU_TWISTS]->mListPartsArr[TW_DISABLECAP + 1], TwistsConfig::isCappyDisableEnabled() ? "Off" : "On", "State");

    msgList[MENU_TWISTS]->mBuffer[TW_ICEPHYSICS].copy(u"Ice Physics");
    al::startAction(optionsList[MENU_TWISTS]->mListPartsArr[TW_ICEPHYSICS + 1], TwistsConfig::isIcePhysicsEnabled() ? "On" : "Off", "State");

    msgList[MENU_TWISTS]->mBuffer[TW_SMALLMARIO].copy(u"Small Mario");
    al::startAction(optionsList[MENU_TWISTS]->mListPartsArr[TW_SMALLMARIO + 1], TwistsConfig::isSmallMarioEnabled() ? "On" : "Off", "State");

    msgList[MENU_TWISTS]->mBuffer[TW_DARKNESS].copy(u"Darkness");
    al::startAction(optionsList[MENU_TWISTS]->mListPartsArr[TW_DARKNESS + 1], TwistsConfig::isDarknessEnabled() ? "On" : "Off", "State");

    msgList[MENU_TWISTS]->mBuffer[TW_TIMEWARP].copy(u"Time Travel");
    al::startAction(optionsList[MENU_TWISTS]->mListPartsArr[TW_TIMEWARP + 1], TwistsConfig::isTimeWarpEnabled() ? "On" : "Off", "State");

    msgList[MENU_TWISTS]->mBuffer[TW_TWOD].copy(u"2D in 3D");
    al::startAction(optionsList[MENU_TWISTS]->mListPartsArr[TW_TWOD + 1], TwistsConfig::isTwoDEnabled() ? "On" : "Off", "State");

    msgList[MENU_TWISTS]->mBuffer[TW_FLUDD].copy(u"F.L.U.D.D.");
    al::startAction(optionsList[MENU_TWISTS]->mListPartsArr[TW_FLUDD + 1], TwistsConfig::isFluddEnabled() ? "On" : "Off", "State");

    msgList[MENU_TWISTS]->mBuffer[TW_MOONGRAVITY].copy(u"Moon Gravity");
    al::startAction(optionsList[MENU_TWISTS]->mListPartsArr[TW_MOONGRAVITY + 1], TwistsConfig::isMoonGravityEnabled() ? "On" : "Off", "State");
}

void StageSceneStateModConfig::exeTwistsSettings() {
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
        case TW_SMALLMARIO:
            TwistsConfig::toggleSmallMario();
            break;
        case TW_DARKNESS:
            TwistsConfig::toggleDarkness();
            break;
        case TW_TIMEWARP:
            TwistsConfig::toggleTimeWarp();
            break;
        case TW_TWOD:
            TwistsConfig::toggleTwoD();
            break;
        case TW_FLUDD:
            TwistsConfig::toggleFludd();
            break;
        case TW_MOONGRAVITY:
            TwistsConfig::toggleMoonGravity();
            break;
        }

        updateTwistsOptions();
        activateInput();
        mIsDecideConfig = false;
    }
}

// ============================================================================
// Misc Menu
// ============================================================================

void StageSceneStateModConfig::initMiscMenu(const al::LayoutInitInfo& initInfo) {
    menuList[MENU_MISC] = new SimpleLayoutMenu("MiscMenu", "OptionModCheck", initInfo, 0, false);
    optionsList[MENU_MISC] = new CommonVerticalList(menuList[MENU_MISC], initInfo, true);
    al::setPaneString(menuList[MENU_MISC], "TxtOption", u"Misc Settings", 0);
    optionsList[MENU_MISC]->initDataNoResetSelected(mMiscMenuOptionsCount);

    setMenuItemCheck(optionsList[MENU_MISC]->mListPartsArr[MISC_SPEEDRUN_MODE + 1]);
    setMenuItemBase(optionsList[MENU_MISC]->mListPartsArr[MISC_SPEEDRUN_CONFIG + 1]);

    optionsList[MENU_MISC]->addStringData(msgList[MENU_MISC]->mBuffer, "TxtContent");
    updateMiscOptions();
}

void StageSceneStateModConfig::updateMiscOptions() {
    msgList[MENU_MISC]->mBuffer[MISC_SPEEDRUN_MODE].copy(u"Speedrun Mode");
    al::startAction(optionsList[MENU_MISC]->mListPartsArr[MISC_SPEEDRUN_MODE + 1], sSpeedrunModeEnabled ? "On" : "Off", "State");

    msgList[MENU_MISC]->mBuffer[MISC_SPEEDRUN_CONFIG].copy(u"Speedrun Config");
}

void StageSceneStateModConfig::exeMiscSettings() {
    if (al::isFirstStep(this)) {
        mCurrentList = optionsList[MENU_MISC];
        mCurrentMenu = menuList[MENU_MISC];
        subMenuStart();
    }

    subMenuUpdate();

    if (mIsDecideConfig && mCurrentList->isDecideEnd()) {
        ChangeStageInfo info =
            ChangeStageInfo(Client::get()->getHolder(), Client::get()->getHolder()->getGameDataFile()->getPlayerStartId().cstr(),
                            GameDataFunction::getCurrentStageName(Client::get()->getHolder()), false, -1, ChangeStageInfo::SubScenarioType::NO_SUB_SCENARIO);
        switch (mCurrentList->mCurSelected) {
        case MISC_SPEEDRUN_MODE:
            sSpeedrunModeEnabled = !sSpeedrunModeEnabled;
            if (sSpeedrunModeEnabled) {
                GameModeBase* mode = GameModeManager::instance()->getMode<GameModeBase>();
                if (mode && mode->isModeActive()) {
                    GameModeManager::instance()->end();
                }
                GameModeManager::instance()->setActive(false);
            }

            Client::get()->getHolder()->changeNextStage(&info, 0);

            updateMiscOptions();
            activateInput();
            break;

        case MISC_SPEEDRUN_CONFIG:
            al::setNerve(this, &NrvStageSceneStateModConfig.SpeedrunConfig);
            break;
        }
    }
}

// ============================================================================
// Speedrun Config Menu
// ============================================================================

void StageSceneStateModConfig::initSpeedrunConfigMenu(const al::LayoutInitInfo& initInfo) {
    menuList[MENU_SPEEDRUN_CONFIG] = new SimpleLayoutMenu("SpeedrunConfigMenu", "OptionModCheck", initInfo, 0, false);
    optionsList[MENU_SPEEDRUN_CONFIG] = new CommonVerticalList(menuList[MENU_SPEEDRUN_CONFIG], initInfo, true);
    al::setPaneString(menuList[MENU_SPEEDRUN_CONFIG], "TxtOption", u"Speedrun Config", 0);
    optionsList[MENU_SPEEDRUN_CONFIG]->initDataNoResetSelected(mSpeedrunConfigOptionsCount);

    setMenuItemCheck(optionsList[MENU_SPEEDRUN_CONFIG]->mListPartsArr[1]);
    setMenuItemBase(optionsList[MENU_SPEEDRUN_CONFIG]->mListPartsArr[2]);
    setMenuItemRoll(optionsList[MENU_SPEEDRUN_CONFIG]->mListPartsArr[3]);

    optionsList[MENU_SPEEDRUN_CONFIG]->addStringData(msgList[MENU_SPEEDRUN_CONFIG]->mBuffer, "TxtContent");
    updateSpeedrunConfigOptions();
}

void StageSceneStateModConfig::updateSpeedrunConfigOptions() {
    msgList[MENU_SPEEDRUN_CONFIG]->mBuffer[SPEEDRUN_NONSTOP].copy(u"Non-Stop (WIP)");
    al::startAction(optionsList[MENU_SPEEDRUN_CONFIG]->mListPartsArr[SPEEDRUN_NONSTOP + 1], sSpeedrunNonStopEnabled ? "On" : "Off", "State");
}

void StageSceneStateModConfig::exeSpeedrunConfig() {
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
            break;
        }

        updateSpeedrunConfigOptions();
        activateInput();
    }
}

// ============================================================================
// Lifecycle Methods
// ============================================================================

void StageSceneStateModConfig::init() {
    initNerve(&NrvStageSceneStateModConfig.MainMenu, 0);
}

void StageSceneStateModConfig::appear() {
    mCurrentMenu->startAppear("Appear");
    al::NerveStateBase::appear();
}

void StageSceneStateModConfig::kill() {
    if (Client::hasServerChanged()) {
        if (Client::get()->mIsAllowReconnect)
            Client::restartConnection();
        Client::showUIMessage(Client::get()->mIsAllowReconnect ? u"Reconnecting..." : u"Server changed. Please restart the game.");
        for (int i = 0; i < 240; i++)
            nn::os::YieldThread();
        Client::hideUIMessage();
    }
    mCurrentMenu->startEnd("End");
    al::NerveStateBase::kill();
}

// ============================================================================
// Option Update Methods
// ============================================================================

void StageSceneStateModConfig::exeSaveData() {
    if (al::isFirstStep(this)) {
        SaveDataAccessFunction::startSaveDataWrite(mGameDataHolder);
    }

    if (SaveDataAccessFunction::updateSaveDataAccess(mGameDataHolder, false)) {
        al::startHitReaction(mCurrentMenu, "リセット", 0);

        mCurrentList->activate();
        mCurrentList->appearCursor();
        al::setNerve(this, &NrvStageSceneStateModConfig.NetworkSettings);
    }
}

// ============================================================================
// Helper Methods
// ============================================================================

void StageSceneStateModConfig::handleMenuInput() {
    mInput->update();
    mCurrentList->update();

    if (mInput->isHoldUiUp() && mInput->isRepeatUiUp()) {
        if (mInput->isTriggerUiUp() && mCurrentList->mCurSelected == mCurrentList->mTopSelectableIdx)
            mCurrentList->jumpBottom();
        mCurrentList->up();
    }

    if (mInput->isHoldUiDown() && mInput->isRepeatUiDown()) {
        if (mInput->isTriggerUiDown() && mCurrentList->mCurSelected == (mCurrentList->mDataCount - 1))
            mCurrentList->jumpTop();
        mCurrentList->down();
    }

    // Only forward left/right to menus (and rows) that actually have roll parts.
    // Calling rollLeft/rollRight on a non-roll item casts to al::RollParts* through
    // a garbage vtable and asserts.
    if (mInput->isTriggerUiLeft() || mInput->isTriggerUiRight()) {
        if (currentMenuHasRollParts()) {
            if (mInput->isTriggerUiLeft())
                mCurrentList->rollLeft();
            if (mInput->isTriggerUiRight())
                mCurrentList->rollRight();
        }
    }

    if (rs::isTriggerUiDecide(mHost))
        deactivateInput();
}

void StageSceneStateModConfig::subMenuStart() {
    mCurrentList->deactivate();
    mCurrentMenu->startEnd("End");
    activateInput();
    mCurrentMenu->startAppear("Appear");
}

void StageSceneStateModConfig::subMenuUpdate() {
    handleMenuInput();

    if (rs::isTriggerUiCancel(mHost) && !mIsDecideConfig) {
        if (mCurrentMenu == menuList[MENU_GAMEMODE] && mShouldHideMessage) {
            Client::hideUIMessage();
            mShouldHideMessage = false;
            mMessageHideTimer = 0;
        }

        // Commit roll parts state only when leaving the menu that owns them,
        // never on arbitrary cancel presses from other menus.
        updateDataFromRollParts();

        if (mCurrentMenu == menuList[MENU_SERVERBROWSER]) {
            endSubMenuToParent(menuList[MENU_NETWORK], optionsList[MENU_NETWORK]);
        } else if (mCurrentMenu == menuList[MENU_GAMEMODE_MODESEL] || mCurrentMenu == menuList[MENU_TWISTS]) {
            endSubMenuToParent(menuList[MENU_GAMEMODE], optionsList[MENU_GAMEMODE]);
        } else if (mCurrentMenu == menuList[MENU_SPEEDRUN_CONFIG]) {
            endSubMenuToParent(menuList[MENU_MISC], optionsList[MENU_MISC]);
        } else if (mGamemodeConfigMenu && mCurrentMenu == mGamemodeConfigMenu->mMenu) {
            endSubMenuToParent(menuList[MENU_GAMEMODE], optionsList[MENU_GAMEMODE]);
        } else {
            endSubMenu();
        }
    }
}

void StageSceneStateModConfig::endSubMenu() {
    if (mCurrentMenu == menuList[MENU_GAMEMODE] && mShouldHideMessage) {
        Client::hideUIMessage();
        mShouldHideMessage = false;
        mMessageHideTimer = 0;
    }

    mCurrentList->deactivate();
    mCurrentMenu->startEnd("End");
    mCurrentList = optionsList[MENU_MAIN];
    mCurrentMenu = menuList[MENU_MAIN];
    mCurrentMenu->startAppear("Appear");
    al::startHitReaction(mCurrentMenu, "リセット", 0);
    al::setNerve(this, &NrvStageSceneStateModConfig.MainMenu);
}

void StageSceneStateModConfig::endSubMenuToParent(SimpleLayoutMenu* parentMenu, CommonVerticalList* parentList) {
    if (mCurrentMenu == menuList[MENU_GAMEMODE] && mShouldHideMessage) {
        Client::hideUIMessage();
        mShouldHideMessage = false;
        mMessageHideTimer = 0;
    }

    mCurrentList->deactivate();
    mCurrentMenu->startEnd("End");
    mCurrentList = parentList;
    mCurrentMenu = parentMenu;
    activateInput();
    mIsDecideConfig = false;

    if (parentMenu == menuList[MENU_GAMEPLAY]) {
        al::setNerve(this, &NrvStageSceneStateModConfig.GameplaySettings);
    } else if (parentMenu == menuList[MENU_NETWORK]) {
        al::setNerve(this, &NrvStageSceneStateModConfig.NetworkSettings);
    } else if (parentMenu == menuList[MENU_GAMEMODE]) {
        al::setNerve(this, &NrvStageSceneStateModConfig.GameModeSettings);
    } else if (parentMenu == menuList[MENU_MISC]) {
        al::setNerve(this, &NrvStageSceneStateModConfig.MiscSettings);
    }
}

void StageSceneStateModConfig::activateInput() {
    mInput->reset();
    mCurrentList->activate();
    mCurrentList->appearCursor();
    mIsDecideConfig = false;
}

void StageSceneStateModConfig::deactivateInput() {
    al::startHitReaction(mCurrentMenu, "決定", 0);
    mCurrentList->endCursor();
    mCurrentList->decide();
    mIsDecideConfig = true;
}

void StageSceneStateModConfig::updateDataFromRollParts() {
    // Only commit gamemode config roll parts when actually in that menu
    if (mGamemodeConfigMenu && mCurrentMenu == mGamemodeConfigMenu->mMenu) {
        mGamemodeConfigMenu->updateDataFromRollParts();
    }

    // Only commit gameplay roll parts when actually in the gameplay menu
    if (mCurrentMenu == menuList[MENU_GAMEPLAY]) {
        int playerColType = ((al::RollParts*)optionsList[MENU_GAMEPLAY]->mListPartsArr[GP_PLAYERCOL + 1])->mSelectedIdx;
        sPuppetBounceEnabled = (playerColType >> 1) & 1;
        sPuppetCollisionEnabled = playerColType & 1;

        int capColType = ((al::RollParts*)optionsList[MENU_GAMEPLAY]->mListPartsArr[GP_CAPCOL + 1])->mSelectedIdx;
        sCapBounceEnabled = (capColType >> 1) & 1;
        sCapCollisionEnabled = capColType & 1;
    }
}