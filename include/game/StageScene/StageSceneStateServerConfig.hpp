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

#include "server/gamemode/GameModeConfigMenu.hpp"
#include "server/gamemode/GameModeConfigMenuFactory.hpp"

class FooterParts;

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

        enum ServerConfigOption {
            GAMEMODECONFIG,
            GAMEMODESWITCH,
            TOGGLESENSORS,
            TOGGLETWISTS,
            SETIP,
            SETPORT,
            TOGGLEMUSIC,
            HIDESERVER,
        };

        virtual al::MessageSystem* getMessageSystem(void) const override;
        virtual void init(void) override;
        virtual void appear(void) override;
        virtual void kill(void) override;

        void exeMainMenu();
        void exeOpenKeyboardIP();
        void exeOpenKeyboardPort();
        void exeHideServer();
        void exeToggleMusic();
        void exeToggleTwists();
        void exeToggleSensors();
        void exeGamemodeConfig();
        void exeGamemodeSelect();
        void exeSaveData();
        void updateSensorsOptions();
        void updateTwistsOptions();
        

        void endSubMenu();

        static bool isCapAttackEnabled();
        static bool isCapReceiveEnabled();
        static bool isPuppetAttackEnabled();
        static bool isPuppetReceiveEnabled();

    private:
        inline void subMenuStart();
        inline void subMenuUpdate();
        inline void subMenuRefresh();
        inline void mainMenuRefresh();

        static bool sCapAttackEnabled;
        static bool sCapReceiveEnabled;
        static bool sPuppetAttackEnabled;
        static bool sPuppetReceiveEnabled;

        // Add these menu components
        SimpleLayoutMenu* mToggleSensorsMenu;
        CommonVerticalList* mToggleSensorsList;
        sead::SafeArray<sead::WFixedSafeString<0x200>, 4>* mToggleSensorsOptions; // 4 options

        al::MessageSystem* mMsgSystem      = nullptr;
        FooterParts*       mFooterParts    = nullptr;
        GameDataHolder*    mGameDataHolder = nullptr;

        InputSeparator* mInput = nullptr;

        SimpleLayoutMenu*   mCurrentMenu = nullptr;
        CommonVerticalList* mCurrentList = nullptr;

        // Root Page, contains buttons for gamemode config, and server ip address changing
        SimpleLayoutMenu*   mMainOptions     = nullptr;
        CommonVerticalList* mMainOptionsList = nullptr;

        // Sub-Page of Mode config, used to select a gamemode for the client to use
        SimpleLayoutMenu*   mModeSelect     = nullptr;
        CommonVerticalList* mModeSelectList = nullptr;

        // Sub-Pages for Mode configuration, has buttons for selecting current gamemode and configuring currently selected mode (if no mode is chosen, button will not do anything)
        struct GameModeEntry {
            GameModeConfigMenu* mMenu;
            SimpleLayoutMenu*   mLayout = nullptr;
            CommonVerticalList* mList   = nullptr;
        };
        sead::SafeArray<GameModeEntry, GameModeConfigMenuFactory::getMenuCount()> mGamemodeConfigMenus;
        GameModeEntry* mGamemodeConfigMenu = nullptr;

        SimpleLayoutMenu* mToggleTwistsMenu;
CommonVerticalList* mToggleTwistsList;
sead::SafeArray<sead::WFixedSafeString<0x200>, 2>* mToggleTwistsOptions; // 1 twist option (Cappy)

        inline void activateInput();
        inline void deactivateInput();

        // Main Menu Options - Updated count
        static constexpr int mMainMenuOptionsCount = 8;
        sead::SafeArray<sead::WFixedSafeString<0x200>, mMainMenuOptionsCount>* mMainMenuOptions = nullptr;
        const sead::WFixedSafeString<0x200>* getMainMenuOptions();

        bool mIsDecideConfig = false;
};

namespace {
    NERVE_HEADER(StageSceneStateServerConfig, MainMenu)
    NERVE_HEADER(StageSceneStateServerConfig, OpenKeyboardIP)
    NERVE_HEADER(StageSceneStateServerConfig, OpenKeyboardPort)
    NERVE_HEADER(StageSceneStateServerConfig, HideServer)
    NERVE_HEADER(StageSceneStateServerConfig, ToggleMusic)
    NERVE_HEADER(StageSceneStateServerConfig, GamemodeConfig)
    NERVE_HEADER(StageSceneStateServerConfig, GamemodeSelect)
    NERVE_HEADER(StageSceneStateServerConfig, ToggleSensors)
    NERVE_HEADER(StageSceneStateServerConfig, ToggleTwists)
    NERVE_HEADER(StageSceneStateServerConfig, SaveData)
}