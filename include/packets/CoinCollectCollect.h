#pragma once

#include "packets/Packet.h"

struct CoinCollectCollect : public Packet {
    PacketType getType() override { return PacketType::COINCOLLECTCOLL; }

    std::vector<u8> serialize() override {
        PacketWriter writer(this);

        writer.writeString(placeID);
        writer.write(worldID);
        writer.writeString(stage);

        return writer.finalize();
    }

    void deserialize(const std::vector<u8>& data) override {
        PacketReader reader(this, data.data(), data.size());

        reader.readString(placeID);
        reader.read(worldID);
        reader.readString(stage);

        reader.finalize();
    }

    sead::FixedSafeString<0x40> placeID;
    int worldID = 0;
    sead::FixedSafeString<0x40> stage;
};
