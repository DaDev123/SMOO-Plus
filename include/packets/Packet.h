#pragma once

#include "nn/account.h"

#include "types.h"

#define PACKBUFSIZE      0x30
#define COSTUMEBUFSIZE   0x20

#define MAXPACKSIZE      0x100

enum PacketType : short {
    UNKNOWN,
    CLIENTINIT,
    PLAYERINF,
    HACKCAPINF,
    GAMEINF,
    GAMEMODEINF,
    PLAYERCON,
    PLAYERDC,
    COSTUMEINF,
    SHINECOLL,
    CAPTUREINF,
    CHANGESTAGE,
    CMD,
    UDPINIT,
    HOLEPUNCH,
    End, // end of enum for bounds checking
};

// attribute otherwise the build log is spammed with unused warnings
USED static const char* packetNames[] = {
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
    "Udp Initialization",
    "Hole punch",
};

enum SenderType {
    SERVER,
    CLIENT,
};

enum ConnectionTypes {
    INIT,
    RECONNECT,
};

struct PACKED Packet {
    nn::account::Uid mUserID; // User ID of the packet owner
    PacketType mType = PacketType::UNKNOWN;
    short mPacketSize = 0; // represents packet size without size of header
};
