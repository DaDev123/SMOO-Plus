#pragma once

#include "al/nerve/HostStateBase.h"
#include "al/message/IUseMessageSystem.h"
#include "al/message/MessageSystem.h"
#include "al/layout/LayoutInitInfo.h"
#include "al/scene/Scene.h"
#include "al/util/NerveUtil.h"

#include "game/Input/InputSeparator.h"
#include "game/Layouts/CommonVerticalList.h"
#include "game/Layouts/SimpleLayoutMenu.h"
#include "game/GameData/GameDataHolder.h"

#include "server/gamemode/GameModeConfigMenu.hpp"
#include "server/gamemode/GameModeConfigMenuFactory.hpp"

#include "TwistsConfig.hpp"

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

class StageSceneStateServerConfig : public al::HostStateBase<al::Scene>, 
                                     public al::IUseMessageSystem {
public:
    enum MainMenuOption {
        NETWORK_SETTINGS,
        GAMEPLAY_SETTINGS,
        GAMEMODE_SETTINGS
    };

    StageSceneStateServerConfig(
        const char* name,
        al::Scene* scene,
        const al::LayoutInitInfo& initInfo,
        FooterParts* footerParts,
        GameDataHolder* dataHolder,
        bool unused
    );

    ~StageSceneStateServerConfig();

    // Lifecycle methods
    virtual void init() override;
    virtual void appear() override;
    virtual void kill() override;
    virtual al::MessageSystem* getMessageSystem() const override;

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
    static bool isCapAttackEnabled();
    static bool isCapReceiveEnabled();
    static bool isPuppetAttackEnabled();
    static bool isPuppetReceiveEnabled();
    static bool isCostumeDoorsUnlocked();
    static bool isLowLatencyEnabled();

private:
    // ========================================================================
    // Menu Initialization
    // ========================================================================
    void initMainMenu(const al::LayoutInitInfo& initInfo);
    void initNetworkMenu(const al::LayoutInitInfo& initInfo);
    void initServerBrowserMenu(const al::LayoutInitInfo& initInfo);
    void initGameplayMenu(const al::LayoutInitInfo& initInfo);
    void initPlayerCollisionMenu(const al::LayoutInitInfo& initInfo);
    void initGameModeMenus(const al::LayoutInitInfo& initInfo);
    void initTwistsMenu(const al::LayoutInitInfo& initInfo);

    // ========================================================================
    // Menu Update Methods
    // ========================================================================
    void updateMainMenuOptions();
    void updateNetworkSettingsOptions();
    void updateGameplaySettingsOptions();
    void updatePlayerCollisionOptions();
    void updateGameModeSettingsOptions();
    void updateTwistsOptions();

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
    static bool sCapAttackEnabled;
    static bool sCapReceiveEnabled;
    static bool sPuppetAttackEnabled;
    static bool sPuppetReceiveEnabled;
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

    // ========================================================================
    // Main Menu
    // ========================================================================
    SimpleLayoutMenu* mMainOptions = nullptr;
    CommonVerticalList* mMainOptionsList = nullptr;
    static constexpr int mMainMenuOptionsCount = 3;
    sead::SafeArray<sead::WFixedSafeString<0x200>, mMainMenuOptionsCount>* mMainMenuOptions = nullptr;

    // ========================================================================
    // Network Settings Menu
    // ========================================================================
    SimpleLayoutMenu* mNetworkMenu = nullptr;
    CommonVerticalList* mNetworkList = nullptr;
    sead::SafeArray<sead::WFixedSafeString<0x200>, 3>* mNetworkOptions = nullptr;

    // ========================================================================
    // Server Browser Menu
    // ========================================================================
    SimpleLayoutMenu* mServerBrowserMenu = nullptr;
    CommonVerticalList* mServerBrowserList = nullptr;
    std::vector<ServerBrowser> mServerBrowserServers;
    int mServerBrowserCount = 0;
    sead::WFixedSafeString<0x200>* mServerBrowserOptions = nullptr;

    // ========================================================================
    // Gameplay Settings Menu
    // ========================================================================
    SimpleLayoutMenu* mGameplayMenu = nullptr;
    CommonVerticalList* mGameplayList = nullptr;
    sead::SafeArray<sead::WFixedSafeString<0x200>, 4>* mGameplayOptions = nullptr;

    // ========================================================================
    // Player Collision Settings Menu
    // ========================================================================
    SimpleLayoutMenu* mPlayerCollisionMenu = nullptr;
    CommonVerticalList* mPlayerCollisionList = nullptr;
    sead::SafeArray<sead::WFixedSafeString<0x200>, 4>* mPlayerCollisionOptions = nullptr;

    // ========================================================================
    // Game Mode Settings Menu
    // ========================================================================
    SimpleLayoutMenu* mGameModeSettingsMenu = nullptr;
    CommonVerticalList* mGameModeSettingsList = nullptr;
    sead::SafeArray<sead::WFixedSafeString<0x200>, 3>* mGameModeSettingsOptions = nullptr;

    // ========================================================================
    // Game Mode Selection Menu
    // ========================================================================
    SimpleLayoutMenu* mModeSelect = nullptr;
    CommonVerticalList* mModeSelectList = nullptr;

    // ========================================================================
    // Game Mode Configuration Menus
    // ========================================================================
    struct GameModeEntry {
        GameModeConfigMenu* mMenu;
        SimpleLayoutMenu* mLayout = nullptr;
        CommonVerticalList* mList = nullptr;
    };
    sead::SafeArray<GameModeEntry, GameModeConfigMenuFactory::getMenuCount()> mGamemodeConfigMenus;
    GameModeEntry* mGamemodeConfigMenu = nullptr;

    // ========================================================================
    // Twists Settings Menu
    // ========================================================================
    SimpleLayoutMenu* mTwistsMenu = nullptr;
    CommonVerticalList* mTwistsList = nullptr;
    sead::SafeArray<sead::WFixedSafeString<0x200>, 3>* mTwistsOptions = nullptr;
};

// ============================================================================
// Nerve Declarations
// ============================================================================

namespace {
    NERVE_HEADER(StageSceneStateServerConfig, MainMenu)
    NERVE_HEADER(StageSceneStateServerConfig, NetworkSettings)
    NERVE_HEADER(StageSceneStateServerConfig, ServerBrowserSelect)
    NERVE_HEADER(StageSceneStateServerConfig, OpenKeyboardIP)
    NERVE_HEADER(StageSceneStateServerConfig, OpenKeyboardPort)
    NERVE_HEADER(StageSceneStateServerConfig, GameplaySettings)
    NERVE_HEADER(StageSceneStateServerConfig, PlayerCollisionSettings)
    NERVE_HEADER(StageSceneStateServerConfig, GameModeSettings)
    NERVE_HEADER(StageSceneStateServerConfig, GameModeConfig)
    NERVE_HEADER(StageSceneStateServerConfig, GameModeSelect)
    NERVE_HEADER(StageSceneStateServerConfig, TwistsSettings)
    NERVE_HEADER(StageSceneStateServerConfig, SaveData)
}