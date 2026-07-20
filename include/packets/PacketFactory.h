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
inline std::unique_ptr<Packet> create(PacketType type) {
    switch (type) {
    case PacketType::CLIENTINIT:
        return std::unique_ptr<Packet>{new (gHeap) InitPacket};

    case PacketType::PLAYERINF:
        return std::unique_ptr<Packet>{new (gHeap) PlayerInf};

    case PacketType::HACKCAPINF:
        return std::unique_ptr<Packet>{new (gHeap) HackCapInf};

    case PacketType::GAMEINF:
        return std::unique_ptr<Packet>{new (gHeap) GameInf};

    case PacketType::PLAYERCON:
        return std::unique_ptr<Packet>{new (gHeap) PlayerConnect};

    case PacketType::PLAYERDC:
        return std::unique_ptr<Packet>{new (gHeap) PlayerDC};

    case PacketType::COSTUMEINF:
        return std::unique_ptr<Packet>{new (gHeap) CostumeInf};

    case PacketType::SHINECOLL:
        return std::unique_ptr<Packet>{new (gHeap) ShineCollect};

    case PacketType::CAPTUREINF:
        return std::unique_ptr<Packet>{new (gHeap) CaptureInf};

    case PacketType::CHANGESTAGE:
        return std::unique_ptr<Packet>{new (gHeap) ChangeStagePacket};

    case PacketType::COINCOLLECTCOLL:
        return std::unique_ptr<Packet>{new (gHeap) CoinCollectCollect};

    case PacketType::CHECKPOINTGET:
        return std::unique_ptr<Packet>{new (gHeap) CheckpointGet};

    case PacketType::MOONROCKHIT:
        return std::unique_ptr<Packet>{new (gHeap) MoonRockHit};

    case PacketType::GAMESTART:
        return std::unique_ptr<Packet>{new (gHeap) GameStart};

    default:
        hk::diag::logLine("Invalid packet type in factory. Dropping packet.");
        return nullptr;
    }
}
}  // namespace PacketFactory