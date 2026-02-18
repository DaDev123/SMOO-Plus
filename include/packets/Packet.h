#pragma once

#include "sead/math/seadQuat.h"    // IWYU pragma: keep
#include "sead/math/seadVector.h"  // IWYU pragma: keep

#include "nn/account.h"
#include "types.h"

#define PACKBUFSIZE 0x30
#define COSTUMEBUFSIZE 0x20
#define MESSAGESIZE 0x4B
#define VERSIONSIZE 0x20  // Change from 0x40 to 0x20 to match C# (32 bytes)

#define MAXPACKSIZE 0x100

enum PacketType : short {
    UNKNOWN,
    CLIENTINIT,
    PLAYERINF,
    HACKCAPINF,
    GAMEINF,
    TAGINF,
    // FREEZEINF,
    PLAYERCON,
    PLAYERDC,
    COSTUMEINF,
    SHINECOLL,
    CAPTUREINF,
    CHANGESTAGE,
    CMD,
    MESSAGE,
    UDPINIT,
    HOLEPUNCH,
    EXTRA,
    HEALTHCOINS,
    COINCOLLECTCOLL,
    End  // end of enum for bounds checking
};

constexpr static const char* packetNames[] = {"Unknown", "Client Initialization", "Player Info", "Player Cap Info", "Game Info", "Tag Info",
                                              //"Freeze Info",
                                              "Player Connect", "Player Disconnect", "Costume Info", "Moon Collection", "Capture Info", "Change Stage",
                                              "Server Command", "Message", "UDP Initialization", "UDP Hole Punch", "Extra", "Health and Coins",
                                              "Purple Coin Collection"};

enum SenderType { SERVER, CLIENT };

enum ConnectionTypes { INIT, RECONNECT };

// unused
/*
static const char *senderNames[] = {
    "Server",
    "Client"
};
*/

struct PACKED Packet {
    nn::account::Uid mUserID;  // User ID of the packet owner
    PacketType mType = PacketType::UNKNOWN;
    short mPacketSize = 0;  // represents packet size without size of header
};

// all packet types

// IWYU pragma: begin_keep
#include "packets/CaptureInf.h"
#include "packets/ChangeStagePacket.h"
#include "packets/CoinCollectCollect.h"
#include "packets/CostumeInf.h"
#include "packets/FreezeInf.h"
#include "packets/GameInf.h"
#include "packets/HackCapInf.h"
#include "packets/HealthCoins.h"
#include "packets/InitPacket.h"
#include "packets/MessagePacket.h"
#include "packets/PlayerConnect.h"
#include "packets/PlayerDC.h"
#include "packets/PlayerInfPacket.h"
#include "packets/ServerCommand.h"
#include "packets/ShineCollect.h"
#include "packets/TagInf.h"
// IWYU pragma: end_keep