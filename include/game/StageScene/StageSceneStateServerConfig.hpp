#pragma once

#include "game/Input/InputSeparator.h"
#include "game/Layouts/CommonVerticalList.h"
#include "game/Layouts/SimpleLayoutMenu.h"
#include "al/message/MessageSystem.h"
#include "al/nerve/HostStateBase.h"
#include "al/message/IUseMessageSystem.h"
#include "al/layout/LayoutInitInfo.h"
#include "al/scene/Scene.h"
#include "al/util/NerveUtil.h"

#include "game/GameData/GameDataHolder.h"

#include "TwistsConfig.hpp"

#include "server/gamemode/GameModeConfigMenu.hpp"
#include "server/gamemode/GameModeConfigMenuFactory.hpp"

#include <vector>

class FooterParts;

// Forward declare the PublicServer struct
struct PublicServer {
    char* name;
    char* ip;
    int port;
    
    PublicServer();
    PublicServer(const char* n, const char* i, int p);
    ~PublicServer();
    PublicServer(const PublicServer& other);
    PublicServer& operator=(const PublicServer& other);
};

class StageSceneStateServerConfig : public al::HostStateBase<al::Scene>, public al::IUseMessageSystem {
public:
    StageSceneStateServerConfig(
        const char*,
        al::Scene*,
        const al::LayoutInitInfo&,
        FooterParts*,
        GameDataHolder*,
        bool
    );

    ~StageSceneStateServerConfig();

    enum MainMenuOption {
        Network_SETTINGS,
        GAMEPLAY_SETTINGS,
        GAMEMODE_SETTINGS
    };

    virtual al::MessageSystem* getMessageSystem(void) const override;
    virtual void init(void) override;
    virtual void appear(void) override;
    virtual void kill(void) override;

    // Menu execution methods
    void exeMainMenu();
    void exeNetworkSettings();
    void exePublicServerSelect();
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
    // Menu helper methods
    void subMenuStart();
    void subMenuUpdate();
    inline void subMenuRefresh();
    void endSubMenu();
    void endSubMenuToParent(SimpleLayoutMenu* parentMenu, CommonVerticalList* parentList);
    void activateInput();
    void deactivateInput();

    // Update methods for menu options
    void updateMainMenuOptions();
    void updateNetworkSettingsOptions();
    void updateGameplaySettingsOptions();
    void updatePlayerCollisionOptions();
    void updateGameModeSettingsOptions();
    void updateTwistsOptions();

    // Static settings (shared across instances)
    static bool sCapAttackEnabled;
    static bool sCapReceiveEnabled;
    static bool sPuppetAttackEnabled;
    static bool sPuppetReceiveEnabled;
    static bool sCostumeDoorsUnlocked;
    static bool sLowLatencyEnabled;

    // Core systems
    al::MessageSystem* mMsgSystem = nullptr;
    FooterParts* mFooterParts = nullptr;
    GameDataHolder* mGameDataHolder = nullptr;
    InputSeparator* mInput = nullptr;

    // Current menu state
    SimpleLayoutMenu* mCurrentMenu = nullptr;
    CommonVerticalList* mCurrentList = nullptr;
    bool mIsDecideConfig = false;

    // Main Menu
    SimpleLayoutMenu* mMainOptions = nullptr;
    CommonVerticalList* mMainOptionsList = nullptr;
    static constexpr int mMainMenuOptionsCount = 3;
    sead::SafeArray<sead::WFixedSafeString<0x200>, mMainMenuOptionsCount>* mMainMenuOptions = nullptr;

    // Network Settings Menu
    SimpleLayoutMenu* mNetworkMenu = nullptr;
    CommonVerticalList* mNetworkList = nullptr;
    sead::SafeArray<sead::WFixedSafeString<0x200>, 3>* mNetworkOptions = nullptr;

    // Public Server Selection Menu (now dynamic!)
    SimpleLayoutMenu* mPublicServerMenu = nullptr;
    CommonVerticalList* mPublicServerList = nullptr;
    std::vector<PublicServer> mPublicServers;  // Dynamic server list
    int mPublicServerCount = 0;
    sead::WFixedSafeString<0x200>* mPublicServerOptions = nullptr;  // Dynamic array

    // Gameplay Settings Menu
    SimpleLayoutMenu* mGameplayMenu = nullptr;
    CommonVerticalList* mGameplayList = nullptr;
    sead::SafeArray<sead::WFixedSafeString<0x200>, 4>* mGameplayOptions = nullptr;

    // Player Collision Settings Menu (submenu of Gameplay)
    SimpleLayoutMenu* mPlayerCollisionMenu = nullptr;
    CommonVerticalList* mPlayerCollisionList = nullptr;
    sead::SafeArray<sead::WFixedSafeString<0x200>, 4>* mPlayerCollisionOptions = nullptr;

    // Game Mode Settings Menu
    SimpleLayoutMenu* mGameModeSettingsMenu = nullptr;
    CommonVerticalList* mGameModeSettingsList = nullptr;
    sead::SafeArray<sead::WFixedSafeString<0x200>, 3>* mGameModeSettingsOptions = nullptr;

    // Game Mode Selection Menu
    SimpleLayoutMenu* mModeSelect = nullptr;
    CommonVerticalList* mModeSelectList = nullptr;

    // Game Mode Configuration Menus
    struct GameModeEntry {
        GameModeConfigMenu* mMenu;
        SimpleLayoutMenu* mLayout = nullptr;
        CommonVerticalList* mList = nullptr;
    };
    sead::SafeArray<GameModeEntry, GameModeConfigMenuFactory::getMenuCount()> mGamemodeConfigMenus;
    GameModeEntry* mGamemodeConfigMenu = nullptr;

    // Twists Settings Menu (now submenu of Game Mode Settings)
    SimpleLayoutMenu* mTwistsMenu = nullptr;
    CommonVerticalList* mTwistsList = nullptr;
    sead::SafeArray<sead::WFixedSafeString<0x200>, 3>* mTwistsOptions = nullptr;
};

namespace {
    NERVE_HEADER(StageSceneStateServerConfig, MainMenu)
    NERVE_HEADER(StageSceneStateServerConfig, NetworkSettings)
    NERVE_HEADER(StageSceneStateServerConfig, PublicServerSelect)
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