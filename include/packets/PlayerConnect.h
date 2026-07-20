#pragma once

#include "sead/prim/seadSafeString.h"

#include "packets/Packet.h"

struct PlayerConnect : public Packet {
    PacketType getType() override { return PacketType::PLAYERCON; }

    std::vector<u8> serialize() override {
        PacketWriter writer(this);

        writer.write(conType);
        writer.write(maxPlayerCount);
        writer.writeString(clientName);

        return writer.finalize();
    }

    void deserialize(const std::vector<u8>& data) override {
        PacketReader reader(this, data.data(), data.size());

        reader.read(conType);
        reader.read(maxPlayerCount);
        reader.readString(clientName);

        reader.finalize();
    }

    ConnectionTypes conType;
    u16 maxPlayerCount = USHRT_MAX;
    sead::FixedSafeString<COSTUMEBUFSIZE> clientName;
};