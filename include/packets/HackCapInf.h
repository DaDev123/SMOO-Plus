#pragma once

#include "packets/Packet.h"

struct HackCapInf : public Packet {
    PacketType getType() override { return PacketType::HACKCAPINF; }

    PacketVector serialize() override {
        PacketWriter writer(this);

        writer.write(capPos.x);
        writer.write(capPos.y);
        writer.write(capPos.z);
        writer.write(capQuat.w);
        writer.write(capQuat.x);
        writer.write(capQuat.y);
        writer.write(capQuat.z);
        writer.write(isCapVisible);
        writer.writeString(capAnim);
        writer.write(capRotQuat.w);
        writer.write(capRotQuat.x);
        writer.write(capRotQuat.y);
        writer.write(capRotQuat.z);

        return writer.finalize();
    }

    void deserialize(const PacketVector& data) override {
        PacketReader reader(this, data.data(), data.size());

        reader.read(capPos.x);
        reader.read(capPos.y);
        reader.read(capPos.z);
        reader.read(capQuat.w);
        reader.read(capQuat.x);
        reader.read(capQuat.y);
        reader.read(capQuat.z);
        reader.read(isCapVisible);
        reader.readString(capAnim);
        reader.read(capRotQuat.w);
        reader.read(capRotQuat.x);
        reader.read(capRotQuat.y);
        reader.read(capRotQuat.z);

        reader.finalize();
    }
    sead::Vector3f capPos;
    sead::Quatf capQuat;
    u8 isCapVisible = false;  // fake bool
    sead::FixedSafeString<PACKBUFSIZE> capAnim;
    sead::Quatf capRotQuat;
};