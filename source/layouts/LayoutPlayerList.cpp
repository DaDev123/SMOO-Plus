#include "layouts/LayoutPlayerList.h"
#include "server/Client.hpp"

sead::BufferedSafeStringBase<char> LayoutPlayerList::getPlayerList() {
    int playerCount = Client::getMaxPlayerCount();

    char playerNameBuf[0x100] = {0}; // max of 16 player names if player name size is 0x10
    sead::BufferedSafeStringBase<char> playerList = sead::BufferedSafeStringBase<char>(playerNameBuf, 0x200);

    if (playerCount <= 0) { return playerList; }

    GameMode gmode = getGameMode();

    // Add your own name to the list at the top
    playerList.appendWithFormat("%s %s\n", getRoleIcon(isMeIt()), Client::instance()->getClientName());

    bool hasOtherGameModes = false;

    // seekers then hiders
    for (int i = 0; i <= 1; i++) {
        bool isIt = i == 0;

        // Add players to the list that are in the same game mode
        for (int j = 0; j < playerCount; j++) {
            PuppetInfo* other = Client::getPuppetInfo(j);

            if (!other || !other->isConnected) { continue; }

            if (other->gameMode != gmode && other->gameMode != GameMode::LEGACY) {
                hasOtherGameModes = true;
                continue;
            }

            if (other->isIt != isIt) { continue; }

            playerList.appendWithFormat("%s %s\n", getRoleIcon(isIt), other->puppetName);
        }
    }

    // Add players to the list that are in different game modes
    if (hasOtherGameModes) {
        playerList.appendWithFormat("\n");
        for (int j = 0; j < playerCount; j++) {
            PuppetInfo* other = Client::getPuppetInfo(j);

            if (!other || !other->isConnected) { continue; }

            if (other->gameMode == gmode || other->gameMode == GameMode::LEGACY) { continue; }

            playerList.appendWithFormat("%s\n", other->puppetName);
        }
    }

    return playerList;
}
