#pragma once

#include "sead/container/seadSafeArray.h"

#include "al/Library/Layout/LayoutActionFunction.h"
#include "al/Library/Layout/LayoutActorUtil.h"
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

class StageSceneStateModConfig : public al::HostStateBase<al::Scene>, public al::IUseMessageSystem {
public:
    StageSceneStateModConfig(const char* name, al::Scene* scene, const al::LayoutInitInfo& initInfo, FooterParts* footerParts, GameDataHolder* dataHolder,
                             bool unused);

    ~StageSceneStateModConfig();

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
    void exeMiscSettings();
    void exeSpeedrunConfig();
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

    // Menu Creation Helpers
    static void setMenuItemBase(al::LayoutActor* item) {
        al::hidePane(item, "WinCheck");
        al::hidePane(item, "Roll");
        al::setPaneLocalSize(item, "TxtContent", {420, 50});
    }

    static void setMenuItemCheck(al::LayoutActor* item) {
        al::showPane(item, "WinCheck");
        al::startAction(item, "Off", "State");
        al::hidePane(item, "Roll");
        al::setPaneLocalSize(item, "TxtContent", {360, 50});
    }

    static void setMenuItemRoll(al::LayoutActor* item) {
        al::hidePane(item, "WinCheck");
        al::showPane(item, "Roll");
        al::setPaneLocalSize(item, "TxtContent", {420, 50});
    }

private:
    static constexpr int menuCount = 10;
    static constexpr int maxMsgCount = 8;
    SimpleLayoutMenu* menuList[menuCount];
    CommonVerticalList* optionsList[menuCount];
    sead::SafeArray<sead::WFixedSafeString<0x200>, maxMsgCount>* msgList[menuCount];
    enum Menu { MENU_MAIN, MENU_NETWORK, MENU_SERVERBROWSER, MENU_GAMEPLAY };

    //@ ============= Main Menu =============
    void initMainMenu(const al::LayoutInitInfo& initInfo);
    void updateMainMenuOptions();

    enum MainMenuOption { MAIN_NETWORK_SETTINGS, MAIN_GAMEPLAY_SETTINGS };
    static constexpr int mMainMenuOptionsCount = 2;

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

    enum GameplayMenuOptions { GP_PLAYERCOL, GP_CAPCOL, GP_COSTUMEDOORS, GP_LATENCY, GP_MUSIC };
    static constexpr int mGameplayMenuOptionsCount = 5;

    //@ ============= Game Mode Menus =============
    bool mShouldHideMessage = false;
    int mMessageHideTimer = 0;

    // ========================================================================
    // Menu Helpers
    // ========================================================================

    // Navigation Helpers
    void handleMenuInput();
    void subMenuStart();
    void subMenuUpdate();
    void endSubMenu();
    void endSubMenuToParent(SimpleLayoutMenu* parentMenu, CommonVerticalList* parentList);
    void activateInput();
    void deactivateInput();

    // Update Helpers
    void updateDataFromRollParts();
    bool currentMenuHasRollParts() const;

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
NERVE_IMPL(StageSceneStateModConfig, MainMenu)
NERVE_IMPL(StageSceneStateModConfig, NetworkSettings)
NERVE_IMPL(StageSceneStateModConfig, ServerBrowserSelect)
NERVE_IMPL(StageSceneStateModConfig, OpenKeyboardIP)
NERVE_IMPL(StageSceneStateModConfig, OpenKeyboardPort)
NERVE_IMPL(StageSceneStateModConfig, GameplaySettings)
NERVE_IMPL(StageSceneStateModConfig, SaveData)

NERVES_MAKE_STRUCT(StageSceneStateModConfig, MainMenu, NetworkSettings, ServerBrowserSelect, OpenKeyboardIP, OpenKeyboardPort, GameplaySettings, SaveData)
}  // namespace