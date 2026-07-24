#pragma once

#include "packets/Packet.h"

struct HackCapInf : public Packet {
    PacketType getType() override { return PacketType::HACKCAPINF; }

    PacketVector serialize() override {
        PacketWriter writer(this);

        writer.write(capPos.x);
        writer.write(capPos.y);
        writer.write(capPos.z);
        writer.write(capJointRot.x);
        writer.write(capJointRot.y);
        writer.write(capJointRot.z);
        writer.write(capJointRot.w);
        writer.write(isCapVisible);
        writer.writeString(capAnim);
        writer.write(capRot.x);
        writer.write(capRot.y);
        writer.write(capRot.z);
        writer.write(capRot.w);

        return writer.finalize();
    }

    void deserialize(const PacketVector& data) override {
        PacketReader reader(this, data.data(), data.size());

        reader.read(capPos.x);
        reader.read(capPos.y);
        reader.read(capPos.z);
        reader.read(capJointRot.x);
        reader.read(capJointRot.y);
        reader.read(capJointRot.z);
        reader.read(capJointRot.w);
        reader.read(isCapVisible);
        reader.readString(capAnim);
        reader.read(capRot.x);
        reader.read(capRot.y);
        reader.read(capRot.z);
        reader.read(capRot.w);

        reader.finalize();
    }
    sead::Vector3f capPos;
    sead::Quatf capJointRot;
    u8 isCapVisible = false;  // fake bool
    sead::FixedSafeString<PACKBUFSIZE> capAnim;
    sead::Quatf capRot;
};