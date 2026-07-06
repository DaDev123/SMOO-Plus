#include "Scene/StageSceneStateModConfig.hpp"

#include "sead/container/seadSafeArray.h"
#include "sead/prim/seadSafeString.h"

#include "al/Library/Layout/LayoutActionFunction.h"
#include "al/Library/Layout/LayoutActorUtil.h"
#include "al/Library/Layout/LayoutInitInfo.h"
#include "al/Library/LiveActor/ActorInitInfo.h"
#include "al/Library/Nerve/NerveUtil.h"
#include "al/Library/Play/Layout/RollParts.h"

#include "game/Layout/CommonVerticalList.h"
#include "game/Layout/FooterParts.h"
#include "game/Layout/SimpleLayoutMenu.h"
#include "game/MapObj/ChangeStageInfo.h"
#include "game/System/GameDataFile.h"
#include "game/System/GameDataFunction.h"
#include "game/System/GameDataHolderAccessor.h"
#include "game/System/SaveDataAccessFunction.h"
#include "game/Util/StageInputFunction.h"

#include <cstdlib>
#include <cstring>

#include "container/seadPtrArray.h"
#include "fsHelper.h"
#include "layouts/PlayerEventLog.h"
#include "server/Client.hpp"

// ============================================================================
// Static Configuration Variables
// ============================================================================

bool StageSceneStateModConfig::sCapCollisionEnabled = true;
bool StageSceneStateModConfig::sCapBounceEnabled = true;
bool StageSceneStateModConfig::sPuppetCollisionEnabled = true;
bool StageSceneStateModConfig::sPuppetBounceEnabled = true;
bool StageSceneStateModConfig::sCostumeDoorsUnlocked = true;
bool StageSceneStateModConfig::sLowLatencyEnabled = false;
bool StageSceneStateModConfig::sSpeedrunModeEnabled = true;
StageSceneStateModConfig::SpeedrunLogLife StageSceneStateModConfig::sLogLife = StageSceneStateModConfig::INF;
bool StageSceneStateModConfig::sShineCountEnabled = true;
bool StageSceneStateModConfig::sSpeedrunNonStopEnabled = false;

// ============================================================================
// ServerBrowser Implementation
// ============================================================================

ServerBrowser::ServerBrowser()
    : name(Client::instance()->mHakkunSceneHeap, ""), ip(Client::instance()->mHakkunSceneHeap, ""), port(0) {}

ServerBrowser::ServerBrowser(const char* n, const char* i, int p)
    : name(Client::instance()->mHakkunSceneHeap, n), ip(Client::instance()->mHakkunSceneHeap, i), port(p) {}

ServerBrowser::ServerBrowser(const ServerBrowser& other)
    : name(Client::instance()->mHakkunSceneHeap, other.name),
      ip(Client::instance()->mHakkunSceneHeap, other.ip), port(other.port) {}

ServerBrowser& ServerBrowser::operator=(const ServerBrowser& other) {
    if (this != &other) {
        name.copy(other.name);
        ip.copy(other.ip);
        port = other.port;
    }
    return *this;
}

// ============================================================================
// Server List Loader
// ============================================================================

void StageSceneStateModConfig::loadServersFromFile() {
    mServerBrowserServers.allocBuffer(256, Client::instance()->mHakkunSceneHeap);
    FsHelper::LoadData loadData = {.path = "sd:/SMOO-Plus/ServerList.txt"};
    if (!FsHelper::isFileExist(loadData.path)) {
        mServerBrowserServers.pushBack(new (Client::instance()->mHakkunSceneHeap)
                                           ServerBrowser("No ServerList.txt found.", "", 0));
        return;
    }
    FsHelper::loadFileFromPath(loadData);

    char* buffer = reinterpret_cast<char*>(loadData.buffer);
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
                mServerBrowserServers.pushBack(new (Client::instance()->mHakkunSceneHeap)
                                                   ServerBrowser(name, ip, port));
            }
        }

        line = strtok_r(nullptr, "\n\r", &savePtr);
    }

    free(loadData.buffer);

    if (mServerBrowserServers.isEmpty()) {
        mServerBrowserServers.pushBack(new (Client::instance()->mHakkunSceneHeap)
                                           ServerBrowser("ERROR: Empty or Invalid File", "", 0));
    }
}

// ============================================================================
// Helper: does the current menu have roll parts on the selected item?
// ============================================================================

bool StageSceneStateModConfig::isRollPartsSelected() const {
    if (mCurrentMenu == menuList[MENU_GAMEPLAY]) {
        switch (mCurrentList->mCurSelected) {
        case GP_PLAYERCOL:
            return true;
        case GP_CAPCOL:
            return true;
        }
        return false;
    }

    if (mCurrentMenu == menuList[MENU_SPEEDRUN_CONFIG]) {
        switch (mCurrentList->mCurSelected) {
        case SPEEDRUN_LOGLIFE:
            return true;
        }
        return false;
    }

    return false;
}

// ============================================================================
// Constructor
// ============================================================================

StageSceneStateModConfig::StageSceneStateModConfig(const char* name, al::Scene* scene,
                                                   const al::LayoutInitInfo& initInfo,
                                                   FooterParts* footerParts, GameDataHolder* dataHolder, bool)
    : al::HostStateBase<al::Scene>(name, scene) {
    mFooterParts = footerParts;
    mGameDataHolder = dataHolder;
    mMsgSystem = initInfo.getMessageSystem();
    mInput = new (Client::instance()->mHakkunSceneHeap) InputSeparator(mHost, true);

    // Load server list
    loadServersFromFile();
    mServerBrowserCount = mServerBrowserServers.size();

    for (int i = 0; i < menuCount; i++) {
        msgList[i] = new (Client::instance()->mHakkunSceneHeap)
            sead::SafeArray<sead::WFixedSafeString<0x200>, maxMsgCount>();
    }

    // Initialize all menus
    initMainMenu(initInfo);
    initNetworkMenu(initInfo);
    initServerBrowserMenu(initInfo);
    initGameplayMenu(initInfo);
    initSpeedrunConfigMenu(initInfo);

    mCurrentList = optionsList[MENU_MAIN];
    mCurrentMenu = menuList[MENU_MAIN];
}

// ============================================================================
// Main Menu
// ============================================================================

void StageSceneStateModConfig::initMainMenu(const al::LayoutInitInfo& initInfo) {
    menuList[MENU_MAIN] = new (Client::instance()->mHakkunSceneHeap)
        SimpleLayoutMenu("ModConfigMenu", "OptionModCheck", initInfo, 0, false);
    optionsList[MENU_MAIN] =
        new (Client::instance()->mHakkunSceneHeap) CommonVerticalList(menuList[MENU_MAIN], initInfo, true);
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
    msgList[MENU_MAIN]->mBuffer[MAIN_SPEEDRUN_SETTINGS].copy(u"Speedrun Settings");
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
        case MAIN_SPEEDRUN_SETTINGS:
            al::setNerve(this, &NrvStageSceneStateModConfig.SpeedrunConfig);
            break;
        }
    }
}

// ============================================================================
// Network Menu
// ============================================================================

void StageSceneStateModConfig::initNetworkMenu(const al::LayoutInitInfo& initInfo) {
    menuList[MENU_NETWORK] = new (Client::instance()->mHakkunSceneHeap)
        SimpleLayoutMenu("NetworkMenu", "OptionModCheck", initInfo, 0, false);
    optionsList[MENU_NETWORK] =
        new (Client::instance()->mHakkunSceneHeap) CommonVerticalList(menuList[MENU_NETWORK], initInfo, true);
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
    msgList[MENU_NETWORK]->mBuffer[NETW_SERVERIP].copy(u"Change Server IP");
    msgList[MENU_NETWORK]->mBuffer[NETW_SERVERPORT].copy(u"Change Server Port");
    msgList[MENU_NETWORK]->mBuffer[NETW_RECONNECT].copy(
        Client::get()->mIsAllowReconnect ? u"Reconnect to Server" : u"Reconnect to Server (Disabled)");
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
        Client::openKeyboardIP();

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
        Client::openKeyboardPort();

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
    menuList[MENU_SERVERBROWSER] = new (Client::instance()->mHakkunSceneHeap)
        SimpleLayoutMenu("ServerBrowserMenu", "OptionModCheck", initInfo, 0, false);
    optionsList[MENU_SERVERBROWSER] = new (Client::instance()->mHakkunSceneHeap)
        CommonVerticalList(menuList[MENU_SERVERBROWSER], initInfo, true);
    al::setPaneString(menuList[MENU_SERVERBROWSER], "TxtOption", u"Server List (SMOO-Plus/ServerList.txt)",
                      0);
    optionsList[MENU_SERVERBROWSER]->initDataNoResetSelected(mServerBrowserCount);

    mServerBrowserOptions =
        new (Client::instance()->mHakkunSceneHeap) sead::WFixedSafeString<0x200>[mServerBrowserCount];
    for (int i = 0; i < mServerBrowserCount; i++) {
        mServerBrowserOptions[i].convertFromMultiByteString(mServerBrowserServers[i]->name,
                                                            (mServerBrowserServers[i]->name.calcLength()));
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
            Client::setServerIP(mServerBrowserServers[selected]->ip.cstr());
            Client::setServerPort(mServerBrowserServers[selected]->port);
            endSubMenuToParent(menuList[MENU_NETWORK], optionsList[MENU_NETWORK]);
        }
    }
}

// ============================================================================
// Gameplay Menu
// ============================================================================

void StageSceneStateModConfig::initGameplayMenu(const al::LayoutInitInfo& initInfo) {
    menuList[MENU_GAMEPLAY] = new (Client::instance()->mHakkunSceneHeap)
        SimpleLayoutMenu("GameplayMenu", "OptionModCheck", initInfo, 0, false);
    optionsList[MENU_GAMEPLAY] = new (Client::instance()->mHakkunSceneHeap)
        CommonVerticalList(menuList[MENU_GAMEPLAY], initInfo, true);
    al::setPaneString(menuList[MENU_GAMEPLAY], "TxtOption", u"Gameplay Settings", 0);
    optionsList[MENU_GAMEPLAY]->initDataNoResetSelected(mGameplayMenuOptionsCount);

    setMenuItemRoll(optionsList[MENU_GAMEPLAY]->mListPartsArr[GP_PLAYERCOL + 1]);
    setMenuItemRoll(optionsList[MENU_GAMEPLAY]->mListPartsArr[GP_CAPCOL + 1]);
    setMenuItemCheck(optionsList[MENU_GAMEPLAY]->mListPartsArr[GP_COSTUMEDOORS + 1]);
    setMenuItemCheck(optionsList[MENU_GAMEPLAY]->mListPartsArr[GP_LATENCY + 1]);
    setMenuItemCheck(optionsList[MENU_GAMEPLAY]->mListPartsArr[GP_MUSIC + 1]);

    optionsList[MENU_GAMEPLAY]->startLoopActionAll("Loop", "Loop");
    RollPartsData* dataColPlayer = new (Client::instance()->mHakkunSceneHeap) RollPartsData(
        4,
        new (Client::instance()->mHakkunSceneHeap)
            const char16_t* [] { u"Off", u"Collision", u"Bounce", u"Collision + Bounce" },
        (sPuppetCollisionEnabled + (sPuppetBounceEnabled << 1)), true);
    RollPartsData* dataColCap = new (Client::instance()->mHakkunSceneHeap) RollPartsData(
        4,
        new (Client::instance()->mHakkunSceneHeap)
            const char16_t* [] { u"Off", u"Collision", u"Bounce", u"Collision + Bounce" },
        (sCapCollisionEnabled + (sCapBounceEnabled << 1)), true);
    RollPartsData* dataEmpty = new (Client::instance()->mHakkunSceneHeap)
        RollPartsData(0, new (Client::instance()->mHakkunSceneHeap) const char16_t* [] { u"" });
    optionsList[MENU_GAMEPLAY]->setRollPartsData(new (Client::instance()->mHakkunSceneHeap) RollPartsData[]{
        *dataColPlayer, *dataColCap, *dataEmpty, *dataEmpty, *dataEmpty});

    optionsList[MENU_GAMEPLAY]->addStringData(msgList[MENU_GAMEPLAY]->mBuffer, "TxtContent");
    updateGameplaySettingsOptions();
}

void StageSceneStateModConfig::updateGameplaySettingsOptions() {
    msgList[MENU_GAMEPLAY]->mBuffer[GP_PLAYERCOL].copy(u"Player Interaction");
    msgList[MENU_GAMEPLAY]->mBuffer[GP_CAPCOL].copy(u"Cappy Interaction");
    msgList[MENU_GAMEPLAY]->mBuffer[GP_COSTUMEDOORS].copy(u"Unlock Costume Doors");
    al::startAction(optionsList[MENU_GAMEPLAY]->mListPartsArr[GP_COSTUMEDOORS + 1],
                    sCostumeDoorsUnlocked ? "On" : "Off", "State");

    msgList[MENU_GAMEPLAY]->mBuffer[GP_LATENCY].copy(u"Reduce Player Latency");
    al::startAction(optionsList[MENU_GAMEPLAY]->mListPartsArr[GP_LATENCY + 1],
                    sLowLatencyEnabled ? "On" : "Off", "State");

    msgList[MENU_GAMEPLAY]->mBuffer[GP_MUSIC].copy(u"In-Game Music");
    al::startAction(optionsList[MENU_GAMEPLAY]->mListPartsArr[GP_MUSIC + 1],
                    Client::isMusicDisabled() ? "Off" : "On", "State");
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
// Speedrun Menu
// ============================================================================

void StageSceneStateModConfig::initSpeedrunConfigMenu(const al::LayoutInitInfo& initInfo) {
    menuList[MENU_SPEEDRUN_CONFIG] = new (Client::instance()->mHakkunSceneHeap)
        SimpleLayoutMenu("SpeedrunMenu", "OptionModCheck", initInfo, nullptr, false);
    optionsList[MENU_SPEEDRUN_CONFIG] = new (Client::instance()->mHakkunSceneHeap)
        CommonVerticalList(menuList[MENU_SPEEDRUN_CONFIG], initInfo, true);
    al::setPaneString(menuList[MENU_SPEEDRUN_CONFIG], "TxtOption", u"Speedrun Settings", 0);
    optionsList[MENU_SPEEDRUN_CONFIG]->initDataNoResetSelected(mSpeedrunConfigOptionsCount);

    setMenuItemRoll(optionsList[MENU_SPEEDRUN_CONFIG]->mListPartsArr[SPEEDRUN_LOGLIFE + 1]);
    setMenuItemCheck(optionsList[MENU_SPEEDRUN_CONFIG]->mListPartsArr[SPEEDRUN_LOG + 1]);
    setMenuItemCheck(optionsList[MENU_SPEEDRUN_CONFIG]->mListPartsArr[SPEEDRUN_SHINECOUNT + 1]);

    // Non-stop likely isn't coming for a while, so I removed it from the menu for this release.
    // setMenuItemCheck(optionsList[MENU_SPEEDRUN_CONFIG]->mListPartsArr[SPEEDRUN_NON_STOP + 1]);

    optionsList[MENU_SPEEDRUN_CONFIG]->startLoopActionAll("Loop", "Loop");

    RollPartsData* dataLogLife = new (Client::instance()->mHakkunSceneHeap) RollPartsData(
        4,
        new (Client::instance()->mHakkunSceneHeap)
            const char16_t* [] { u"Never", u"After 15 Seconds", u"After 10 Seconds", u"After 5 Seconds" },
        sLogLife, true);
    RollPartsData* dataEmpty = new (Client::instance()->mHakkunSceneHeap)
        RollPartsData(0, new (Client::instance()->mHakkunSceneHeap) const char16_t* [] { u"" });

    optionsList[MENU_SPEEDRUN_CONFIG]->setRollPartsData(
        new (Client::instance()->mHakkunSceneHeap) RollPartsData[]{*dataLogLife, *dataEmpty, *dataEmpty});

    optionsList[MENU_SPEEDRUN_CONFIG]->addStringData(msgList[MENU_SPEEDRUN_CONFIG]->mBuffer, "TxtContent");
    updateSpeedrunConfig();
}

void StageSceneStateModConfig::updateSpeedrunConfig() {
    // Non-stop likely isn't coming for a while, so I removed it from the menu for this release.
    // msgList[MENU_SPEEDRUN_CONFIG]->mBuffer[SPEEDRUN_NON_STOP].copy(u"Nonstop (WIP)");
    // al::startAction(optionsList[MENU_SPEEDRUN_CONFIG]->mListPartsArr[SPEEDRUN_NON_STOP + 1],
    //                sSpeedrunNonStopEnabled ? "On" : "Off", "State");

    msgList[MENU_SPEEDRUN_CONFIG]->mBuffer[SPEEDRUN_LOGLIFE].copy(u"Clear Log Entries");

    msgList[MENU_SPEEDRUN_CONFIG]->mBuffer[SPEEDRUN_LOG].copy(u"Player Event Log");
    al::startAction(optionsList[MENU_SPEEDRUN_CONFIG]->mListPartsArr[SPEEDRUN_LOG + 1],
                    PlayerEventLog::isShow() ? "On" : "Off", "State");

    msgList[MENU_SPEEDRUN_CONFIG]->mBuffer[SPEEDRUN_SHINECOUNT].copy(u"Moon Counter");
    al::startAction(optionsList[MENU_SPEEDRUN_CONFIG]->mListPartsArr[SPEEDRUN_SHINECOUNT + 1],
                    sShineCountEnabled ? "On" : "Off", "State");
}

void StageSceneStateModConfig::exeSpeedrunConfig() {
    if (al::isFirstStep(this)) {
        mCurrentList = optionsList[MENU_SPEEDRUN_CONFIG];
        mCurrentMenu = menuList[MENU_SPEEDRUN_CONFIG];
        subMenuStart();
        updateSpeedrunConfig();
    }

    subMenuUpdate();

    if (mIsDecideConfig && mCurrentList->isDecideEnd()) {
        // ChangeStageInfo info =
        //     ChangeStageInfo(Client::get()->getHolder(),
        //                     Client::get()->getHolder()->getGameDataFile()->getPlayerStartId().cstr(),
        //                     GameDataFunction::getCurrentStageName(Client::get()->getHolder()), false, -1,
        //                     ChangeStageInfo::SubScenarioType::NO_SUB_SCENARIO);
        switch (mCurrentList->mCurSelected) {
        case SPEEDRUN_NON_STOP:
            sSpeedrunNonStopEnabled = false;
            // Client::get()->getHolder()->changeNextStage(&info, 0);
            break;
        case SPEEDRUN_LOG:
            PlayerEventLog::toggleShow();
            break;
        case SPEEDRUN_SHINECOUNT:
            sShineCountEnabled = !sShineCountEnabled;
            break;
        }
        updateSpeedrunConfig();
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
        Client::showUIMessage(Client::get()->mIsAllowReconnect ? u"Reconnecting..." :
                                                                 u"Server changed. Please restart the game.");
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

    // Only forward left/right to menus (and rows) if roll parts are selected.
    // Calling rollLeft/rollRight on a non-roll item casts to al::RollParts* through
    // a garbage vtable and asserts.
    if (isRollPartsSelected()) {
        if (mInput->isTriggerUiLeft())
            mCurrentList->rollLeft();
        if (mInput->isTriggerUiRight())
            mCurrentList->rollRight();

        // Early return so that you can't "decide" on roll parts.
        return;
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
        // Commit roll parts state only when leaving the menu that owns them,
        // never on arbitrary cancel presses from other menus.
        updateDataFromRollParts();

        if (mCurrentMenu == menuList[MENU_SERVERBROWSER]) {
            endSubMenuToParent(menuList[MENU_NETWORK], optionsList[MENU_NETWORK]);

        } else {
            endSubMenu();
        }
    }
}

void StageSceneStateModConfig::endSubMenu() {
    mCurrentList->deactivate();
    mCurrentMenu->startEnd("End");
    mCurrentList = optionsList[MENU_MAIN];
    mCurrentMenu = menuList[MENU_MAIN];
    mCurrentMenu->startAppear("Appear");
    al::startHitReaction(mCurrentMenu, "リセット", 0);
    al::setNerve(this, &NrvStageSceneStateModConfig.MainMenu);
}

void StageSceneStateModConfig::endSubMenuToParent(SimpleLayoutMenu* parentMenu,
                                                  CommonVerticalList* parentList) {
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
    } else if (parentMenu == menuList[MENU_SPEEDRUN_CONFIG]) {
        al::setNerve(this, &NrvStageSceneStateModConfig.SpeedrunConfig);
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
    if (mCurrentMenu == menuList[MENU_GAMEPLAY]) {
        s32 playerColType =
            ((al::RollParts*)optionsList[MENU_GAMEPLAY]->mListPartsArr[GP_PLAYERCOL + 1])->mSelectedIdx;
        sPuppetBounceEnabled = (playerColType >> 1) & 1;
        sPuppetCollisionEnabled = playerColType & 1;

        s32 capColType =
            ((al::RollParts*)optionsList[MENU_GAMEPLAY]->mListPartsArr[GP_CAPCOL + 1])->mSelectedIdx;
        sCapBounceEnabled = (capColType >> 1) & 1;
        sCapCollisionEnabled = capColType & 1;
    }

    if (mCurrentMenu == menuList[MENU_SPEEDRUN_CONFIG]) {
        sLogLife = (SpeedrunLogLife)((al::RollParts*)optionsList[MENU_SPEEDRUN_CONFIG]
                                         ->mListPartsArr[SPEEDRUN_LOGLIFE + 1])
                       ->mSelectedIdx;
        // recreate log in case its settings were adjusted
        if (PlayerEventLog::sInstance) {
            delete PlayerEventLog::sInstance;
            PlayerEventLog::sInstance = new (Client::instance()->mHakkunSceneHeap) PlayerEventLog();
        }
    }
}