#pragma once

#include "hk/util/Math.h"

#include "al/Library/Layout/LayoutInitInfo.h"
#include "al/Library/Message/IUseMessageSystem.h"
#include "al/Library/Message/MessageSystem.h"
#include "al/Library/Nerve/NerveSetupUtil.h"
#include "al/Library/Nerve/NerveStateBase.h"
#include "al/Library/Scene/Scene.h"

#include "game/Input/InputSeparator.h"
#include "game/Layout/CommonVerticalList.h"
#include "game/Layout/SimpleLayoutMenu.h"
#include "game/System/GameDataHolder.h"

#include <vector>

#include "container/seadSafeArray.h"
#include "server/gamemode/GameModeConfigMenuFactory.hpp"

class FooterParts;

// ============================================================================
// Server Browser Structure
// ============================================================================

struct ServerBrowser {
    char* name;
    char* ip;
    int port;

    ServerBrowser();
    ServerBrowser(const char* n, const char* i, int p);
    ~ServerBrowser();
    ServerBrowser(const ServerBrowser& other);
    ServerBrowser& operator=(const ServerBrowser& other);
};

// ============================================================================
// Main Configuration State Class
// ============================================================================

class StageSceneStateServerConfig : public al::HostStateBase<al::Scene>, public al::IUseMessageSystem {
public:
    StageSceneStateServerConfig(const char* name, al::Scene* scene, const al::LayoutInitInfo& initInfo, FooterParts* footerParts, GameDataHolder* dataHolder,
                                bool unused);

    ~StageSceneStateServerConfig();

    // Lifecycle methods
    virtual void init() override;
    virtual void appear() override;
    virtual void kill() override;
    virtual al::MessageSystem* getMessageSystem() const override { return mMsgSystem; };

    // Menu execution methods
    void exeMainMenu();
    void exeNetworkSettings();
    void exeServerBrowserSelect();
    void exeOpenKeyboardIP();
    void exeOpenKeyboardPort();
    void exeGameplaySettings();
    void exePlayerCollisionSettings();
    void exeGameModeSettings();
    void exeGameModeConfig();
    void exeGameModeSelect();
    void exeTwistsSettings();
    void exeSaveData();

    // Static getters for settings
    static bool isCapCollisionEnabled() { return sCapCollisionEnabled; };
    static bool isCapBounceEnabled() { return sCapBounceEnabled; };
    static bool isPuppetCollisionEnabled() { return sPuppetCollisionEnabled; };
    static bool isPuppetBounceEnabled() { return sPuppetBounceEnabled; };
    static bool isCostumeDoorsUnlocked() { return sCostumeDoorsUnlocked; };
    static bool isLowLatencyEnabled() { return sLowLatencyEnabled; };

    // Static setters for settings
    static void setCapCollisionEnabled(bool enabled) { sCapCollisionEnabled = enabled; };
    static void setCapBounceEnabled(bool enabled) { sCapBounceEnabled = enabled; };
    static void setPuppetCollisionEnabled(bool enabled) { sPuppetCollisionEnabled = enabled; };
    static void setPuppetBounceEnabled(bool enabled) { sPuppetBounceEnabled = enabled; };
    static void setCostumeDoorsUnlocked(bool unlocked) { sCostumeDoorsUnlocked = unlocked; };
    static void setLowLatencyEnabled(bool enabled) { sLowLatencyEnabled = enabled; };

private:
    static constexpr int menuCount = 8;
    static constexpr int maxMsgCount = 8;
    SimpleLayoutMenu* menuList[menuCount];
    CommonVerticalList* optionsList[menuCount];
    sead::SafeArray<sead::WFixedSafeString<0x200>, maxMsgCount>* msgList[menuCount];
    enum Menus { MENU_MAIN, MENU_NETWORK, MENU_SERVERBROWSER, MENU_GAMEPLAY, MENU_PLAYERCOLLISION, MENU_GAMEMODE, MENU_GAMEMODE_MODESEL, MENU_TWISTS };

    //@ ============= Main Menu =============
    void initMainMenu(const al::LayoutInitInfo& initInfo);
    void updateMainMenuOptions();

    enum MainMenuOption { MAIN_NETWORK_SETTINGS, MAIN_GAMEPLAY_SETTINGS, MAIN_GAMEMODE_SETTINGS };
    static constexpr int mMainMenuOptionsCount = 3;

    //@ ============= Network Menu =============
    void initNetworkMenu(const al::LayoutInitInfo& initInfo);
    void updateNetworkSettingsOptions();

    enum NetworkMenuOption { NETW_SERVERLIST, NETW_SERVERIP, NETW_SERVERPORT, NETW_RECONNECT };
    static constexpr int mNetworkMenuOptionsCount = 4;

    //@ ============= Server Browser Menu =============
    void initServerBrowserMenu(const al::LayoutInitInfo& initInfo);

    std::vector<ServerBrowser> mServerBrowserServers;
    int mServerBrowserCount = 0;
    sead::WFixedSafeString<0x200>* mServerBrowserOptions = nullptr;

    //@ ============= Gameplay Menu =============
    void initGameplayMenu(const al::LayoutInitInfo& initInfo);
    void updateGameplaySettingsOptions();

    enum GameplayMenuOptions { GP_PLAYERCOLLISION, GP_COSTUMEDOORS, GP_LATENCY, GP_MUSIC };
    static constexpr int mGameplayMenuOptionsCount = 4;

    //@ ============= Player Collision Menu =============
    void initPlayerCollisionMenu(const al::LayoutInitInfo& initInfo);
    void updatePlayerCollisionOptions();

    enum PlayerCollisionMenuOptions { PC_CAPCOLLISION, PC_CAPBOUNCE, PC_PLAYERCOLLISION, PC_PLAYERBOUNCE };
    static constexpr int mPlayerCollisionMenuOptionsCount = 4;

    //@ ============= Game Mode Menus =============
    void initGameModeMenus(const al::LayoutInitInfo& initInfo);
    void updateGameModeSettingsOptions();

    // Game Mode Settings Menu
    enum GameModeMenuOptions { GM_MODESETTINGS, GM_TWISTS, GM_MODESELECT };
    static constexpr int mGameModeMenuOptionsCount = 3;

    // Game Mode Configuration Menus
    struct GameModeEntry {
        GameModeConfigMenu* mMenu;
        SimpleLayoutMenu* mLayout = nullptr;
        CommonVerticalList* mList = nullptr;
    };
    sead::SafeArray<GameModeEntry, GameModeConfigMenuFactory::getMenuCount()> mGamemodeConfigMenus;
    GameModeEntry* mGamemodeConfigMenu = nullptr;

    //@ ============= Twists Menu =============
    void initTwistsMenu(const al::LayoutInitInfo& initInfo);
    void updateTwistsOptions();

    enum TwistsMenuOptions { TW_DISABLECAP, TW_ICEPHYSICS, TW_MORESOON };
    static constexpr int mTwistsMenuOptionsCount = 3;

    // ========================================================================
    // Menu Navigation Helpers
    // ========================================================================
    void handleMenuInput();
    void subMenuStart();
    void subMenuUpdate();
    void subMenuRefresh();
    void refreshMenu(CommonVerticalList* list, sead::WFixedSafeString<0x200>* options, int count);
    void endSubMenu();
    void endSubMenuToParent(SimpleLayoutMenu* parentMenu, CommonVerticalList* parentList);
    void activateInput();
    void deactivateInput();

    // ========================================================================
    // Static Configuration
    // ========================================================================
    static bool sCapCollisionEnabled;
    static bool sCapBounceEnabled;
    static bool sPuppetCollisionEnabled;
    static bool sPuppetBounceEnabled;
    static bool sCostumeDoorsUnlocked;
    static bool sLowLatencyEnabled;

    // ========================================================================
    // Core Systems
    // ========================================================================
    al::MessageSystem* mMsgSystem = nullptr;
    FooterParts* mFooterParts = nullptr;
    GameDataHolder* mGameDataHolder = nullptr;
    InputSeparator* mInput = nullptr;

    // ========================================================================
    // Current Menu State
    // ========================================================================
    SimpleLayoutMenu* mCurrentMenu = nullptr;
    CommonVerticalList* mCurrentList = nullptr;
    bool mIsDecideConfig = false;
};

// ============================================================================
// Nerve Declarations
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

NERVES_MAKE_STRUCT(StageSceneStateServerConfig, MainMenu, NetworkSettings, ServerBrowserSelect, OpenKeyboardIP, OpenKeyboardPort, GameplaySettings,
                   PlayerCollisionSettings, GameModeSettings, GameModeConfig, GameModeSelect, TwistsSettings, SaveData)
}  // namespace