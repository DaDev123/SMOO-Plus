#pragma once

#include "packets/Packet.h"

struct GameStart : public Packet {
    PacketType getType() override { return PacketType::GAMESTART; }

    std::vector<u8> serialize() override {
        PacketWriter writer(this);

        return writer.finalize();
    }

    void deserialize(const std::vector<u8>& data) override {
        PacketReader reader(this, data.data(), data.size());

        reader.finalize();
    }
};