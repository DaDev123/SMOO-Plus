#pragma once

#include "Packet.h"
#include "server/Client.hpp"

template <typename UpdateType>
struct PACKED GameModeInf : Packet {
    GameModeInf() : Packet() {
        this->mType = PacketType::GAMEMODEINF;
        mPacketSize = sizeof(GameModeInf) - sizeof(Packet);
    };
    u8 mModeAndType = 0;

    GameMode gameMode() {
        GameMode mode = (GameMode)((((mModeAndType >> 4) + 1) % 16) - 1);

        if (mode == GameMode::LEGACY) {
            u8 type = mModeAndType & 0x0f;

            // STATE and TIME (H&S or Sardines)
            if (type == 3) {
                return GameMode::LEGACY;
            }

            // ROUNDCANCEL or FALLOFF (Freeze-Tag)
            if (type == 4 || type == 8) {
                return GameMode::FREEZETAG;
            }

            // (ROUNDSTART or STATE) or (PLAYER or TIME)
            if (type == 1 || type == 2) {
                /**
                 * These types could in the past overlap between different gamemodes.
                 *
                 * Individual STATE or TIME updates are only send by the server
                 * when using `tag` server commands.
                 *
                 * If the user ID is for another user, assume it's for Freeze-Tag.
                 *
                 * This could be wrong, though. I assume here that the `tag` server
                 * commands are unlikely to get used while playing Freeze-Tag,
                 * because they don't make sense for it. (They could cause issues
                 * for Freeze-Tag).
                 *
                 * When playing H&S or Sardines this will result in those updates
                 * for other players are ignored. But the update for ourselves will get trough.
                 *
                 * Newer client versions in H&S or Sardines mode will resend their
                 * combined STATUS and TIME after receiving such an update for themselves,
                 * so that missing update should be auto corrected. When playing
                 * together with older client versions, using the `tag` server
                 * commands could cause issues because of this.
                 */
                if (this->mUserID != Client::getClientId()) {
                    return GameMode::FREEZETAG;
                }
            }
        }

        return mode;
    }

    UpdateType updateType() {
        return static_cast<UpdateType>(mModeAndType & 0x0f);
    }

    void setGameMode(GameMode mode) {
        mModeAndType = (mode << 4) | (mModeAndType & 0x0f);
    }

    void setUpdateType(UpdateType type) {
        mModeAndType = (mModeAndType & 0xf0) | (type & 0x0f);
    }
};

struct PACKED DisabledGameModeInf : GameModeInf<u8> {
    DisabledGameModeInf(nn::account::Uid userID) : GameModeInf() {
        setGameMode(GameMode::NONE);
        setUpdateType(3); // so that legacy Hide&Seek and Sardines clients will parse isIt = false
        mUserID     = userID;
        mPacketSize = sizeof(DisabledGameModeInf) - sizeof(Packet);
    };
    bool isIt    = false;
    u8   seconds = 0;
    u16  minutes = 0;
};
