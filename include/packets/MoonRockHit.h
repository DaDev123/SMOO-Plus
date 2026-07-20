#pragma once

#include "packets/Packet.h"

struct MoonRockHit : public Packet {
    PacketType getType() override { return PacketType::MOONROCKHIT; }

    std::vector<u8> serialize() override {
        PacketWriter writer(this);

        writer.write(worldId);

        return writer.finalize();
    }

    void deserialize(const std::vector<u8>& data) override {
        PacketReader reader(this, data.data(), data.size());

        reader.read(worldId);

        reader.finalize();
    }

    int worldId = 0;
};