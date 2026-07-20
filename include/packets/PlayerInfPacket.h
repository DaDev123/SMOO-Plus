#pragma once

#include "algorithms/PlayerAnims.h"
#include "packets/Packet.h"

struct PlayerInf : public Packet {
    PacketType getType() override { return PacketType::PLAYERINF; }

    std::vector<u8> serialize() override {
        PacketWriter writer(this);

        writer.write(playerPos.x);
        writer.write(playerPos.y);
        writer.write(playerPos.z);
        writer.write(playerRot.w);
        writer.write(playerRot.x);
        writer.write(playerRot.y);
        writer.write(playerRot.z);
        writer.write(animBlendWeights[0]);
        writer.write(animBlendWeights[1]);
        writer.write(animBlendWeights[2]);
        writer.write(animBlendWeights[3]);
        writer.write(animBlendWeights[4]);
        writer.write(animBlendWeights[5]);
        writer.write(actName);
        writer.write(subActName);

        return writer.finalize();
    }

    void deserialize(const std::vector<u8>& data) override {
        PacketReader reader(this, data.data(), data.size());

        reader.read(playerPos.x);
        reader.read(playerPos.y);
        reader.read(playerPos.z);
        reader.read(playerRot.w);
        reader.read(playerRot.x);
        reader.read(playerRot.y);
        reader.read(playerRot.z);
        reader.read(animBlendWeights[0]);
        reader.read(animBlendWeights[1]);
        reader.read(animBlendWeights[2]);
        reader.read(animBlendWeights[3]);
        reader.read(animBlendWeights[4]);
        reader.read(animBlendWeights[5]);
        reader.read(actName);
        reader.read(subActName);

        reader.finalize();
    }

    sead::Vector3f playerPos;
    sead::Quatf playerRot;
    float animBlendWeights[6];
    PlayerAnims::Type actName;
    PlayerAnims::Type subActName;

    bool operator==(const PlayerInf& rhs) const {
        bool isWeightsEqual = true;
        for (size_t i = 0; i < 6; i++) {
            if (animBlendWeights[i] != rhs.animBlendWeights[i]) {
                isWeightsEqual = false;
                break;
            }
        }
        return (playerPos == rhs.playerPos && playerRot == rhs.playerRot && isWeightsEqual &&
                actName == rhs.actName && subActName == rhs.subActName);
    }

    bool operator!=(const PlayerInf& rhs) const { return !operator==(rhs); }
};