#pragma once

#include "sead/math/seadVector.h"
#include "sead/math/seadQuat.h"

#include "nn/account.h"

#include "types.h"

#define PACKBUFSIZE      0x30
#define COSTUMEBUFSIZE   0x20

#define MAXPACKSIZE      0x100

enum PacketType : short {
        UNKNOWN,        // = 0
        CLIENTINIT,     // = 1
        PLAYERINF,      // = 2
        HACKCAPINF,     // = 3 
        GAMEINF,        // = 4
        GAMEMODEINF,    // = 5 
        PLAYERCON,      // = 6
        PLAYERDC,       // = 7 
        COSTUMEINF,     // = 8
        SHINECOLL,      // = 9
        CAPTUREINF,     // = 10
        CHANGESTAGE,    // = 11
        CMD,            // = 12
        EXTRA,          // = 15
        HEALTH_COINS,   // = 16 
        COSTUMESEND,    // = 17
        End             // end of enum for bounds checking
};

// attribute otherwise the build log is spammed with unused warnings
USED static const char *packetNames[] = {
    "Unknown",
    "Client Initialization",
    "Player Info",
    "Player Cap Info",
    "Game Info",
    "Gamemode Info",
    "Player Connect",
    "Player Disconnect",
    "Costume Info",
    "Moon Collection",
    "Capture Info",
    "Change Stage",
    "Server Command",
    "Extras Packets (infCapDives & Noclip)",
    "Health and Coins",
    "Send Costume"
};

enum SenderType {
    SERVER,
    CLIENT
};

enum ConnectionTypes {
    INIT,
    RECONNECT
};

// unused
/*
static const char *senderNames[] = {
    "Server",
    "Client"
};
*/

struct PACKED Packet {
    nn::account::Uid mUserID; // User ID of the packet owner
    PacketType mType = PacketType::UNKNOWN;
    short mPacketSize = 0; // represents packet size without size of header
};

// all packet types

#include "packets/PlayerInfPacket.h"
#include "packets/PlayerConnect.h"
#include "packets/PlayerDC.h"
#include "packets/GameInf.h"
#include "packets/CostumeInf.h"
#include "packets/ServerCommand.h"
#include "packets/ShineCollect.h"
#include "packets/CaptureInf.h"
#include "packets/HackCapInf.h"
#include "packets/ChangeStagePacket.h"
#include "packets/InitPacket.h"