#pragma once

#include "hk/diag/diag.h"

#include <experimental/memory>

#include "main.hpp"
#include "packets/CaptureInf.h"
#include "packets/ChangeStagePacket.h"
#include "packets/CheckpointGet.h"
#include "packets/CoinCollectCollect.h"
#include "packets/CostumeInf.h"
#include "packets/GameInf.h"
#include "packets/GameStart.h"
#include "packets/HackCapInf.h"
#include "packets/InitPacket.h"
#include "packets/MoonRockHit.h"
#include "packets/Packet.h"
#include "packets/PlayerConnect.h"
#include "packets/PlayerDC.h"
#include "packets/PlayerInfPacket.h"
#include "packets/ShineCollect.h"

namespace PacketFactory {
inline Packet* create(PacketType type) {
    switch (type) {
    case PacketType::CLIENTINIT:
        return new (gHeap) InitPacket;

    case PacketType::PLAYERINF:
        return new (gHeap) PlayerInf;

    case PacketType::HACKCAPINF:
        return new (gHeap) HackCapInf;

    case PacketType::GAMEINF:
        return new (gHeap) GameInf;

    case PacketType::PLAYERCON:
        return new (gHeap) PlayerConnect;

    case PacketType::PLAYERDC:
        return new (gHeap) PlayerDC;

    case PacketType::COSTUMEINF:
        return new (gHeap) CostumeInf;

    case PacketType::SHINECOLL:
        return new (gHeap) ShineCollect;

    case PacketType::CAPTUREINF:
        return new (gHeap) CaptureInf;

    case PacketType::CHANGESTAGE:
        return new (gHeap) ChangeStagePacket;

    case PacketType::COINCOLLECTCOLL:
        return new (gHeap) CoinCollectCollect;

    case PacketType::CHECKPOINTGET:
        return new (gHeap) CheckpointGet;

    case PacketType::MOONROCKHIT:
        return new (gHeap) MoonRockHit;

    case PacketType::GAMESTART:
        return new (gHeap) GameStart;

    default:
        hk::diag::logLine("Invalid packet type in factory. Dropping packet.");
        return nullptr;
    }
}
}  // namespace PacketFactory