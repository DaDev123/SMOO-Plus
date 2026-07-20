#pragma once

#include "packets/Packet.h"

struct CostumeInf : public Packet {
    PacketType getType() override { return PacketType::COSTUMEINF; }

    std::vector<u8> serialize() override {
        PacketWriter writer(this);

        writer.writeString(bodyModel);
        writer.writeString(capModel);

        return writer.finalize();
    }

    void deserialize(const std::vector<u8>& data) override {
        PacketReader reader(this, data.data(), data.size());

        reader.readString(bodyModel);
        reader.readString(capModel);

        reader.finalize();
    }

    sead::FixedSafeString<COSTUMEBUFSIZE> bodyModel;
    sead::FixedSafeString<COSTUMEBUFSIZE> capModel;
};