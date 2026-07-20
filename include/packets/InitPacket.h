#pragma once

#include "packets/Packet.h"

struct InitPacket : public Packet {
    PacketType getType() override { return PacketType::CLIENTINIT; }

    std::vector<u8> serialize() override {
        PacketWriter writer(this);

        writer.write(maxPlayers);

        return writer.finalize();
    }

    void deserialize(const std::vector<u8>& data) override {
        PacketReader reader(this, data.data(), data.size());

        reader.read(maxPlayers);

        reader.finalize();
    }

    u16 maxPlayers = 0;
};