#pragma once

#include "server/gamemode/GameMode.hpp"

class LayoutPlayerList {
    protected:
        virtual const char* getRoleIcon(bool isIt);
        virtual GameMode getGameMode();
        virtual bool isMeIt();

        sead::BufferedSafeStringBase<char> getPlayerList();
};
