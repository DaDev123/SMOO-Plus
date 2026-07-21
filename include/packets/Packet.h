#pragma once

#include "hk/diag/diag.h"

#include "nn/account.h"

#include "sead/math/seadQuat.h"    // IWYU pragma: keep
#include "sead/math/seadVector.h"  // IWYU pragma: keep
#include "sead/prim/seadSafeString.h"

#include <cstring>
#include <vector>

#include "main.hpp"

#define PACKBUFSIZE 0x30
#define COSTUMEBUFSIZE 0x20
#define MESSAGESIZE 0x4B

#define MAXPACKSIZE 0x100

enum PacketType : short {
    UNKNOWN,
    CLIENTINIT,
    PLAYERINF,
    HACKCAPINF,
    GAMEINF,
    TAGINF,
    PLAYERCON,
    PLAYERDC,
    COSTUMEINF,
    SHINECOLL,
    CAPTUREINF,
    CHANGESTAGE,
    CMD,
    COINCOLLECTCOLL,
    CHECKPOINTGET,
    MOONROCKHIT,
    GAMESTART,
    End  // end of enum for bounds checking
};

constexpr static const char* packetNames[] = {"Unknown",        "Client Initialization",
                                              "Player Info",    "Player Cap Info",
                                              "Game Info",      "Tag Info",
                                              "Player Connect", "Player Disconnect",
                                              "Costume Info",   "Moon Collection",
                                              "Capture Info",   "Change Stage",
                                              "Server Command", "Regional Coin Collection",
                                              "Checkpoint Get", "Moon Rock Hit",
                                              "Game Start"};

enum ConnectionTypes { INIT, RECONNECT };

// uid part 1 + uid part 2 + packet type + packet size
static constexpr s32 sHeaderSize = sizeof(u64) + sizeof(u64) + sizeof(PacketType) + sizeof(short);

template <typename T>
class PacketAllocator {
public:
    using value_type = T;

    PacketAllocator() noexcept = default;

    template <typename U>
    PacketAllocator(const PacketAllocator<U>& other) noexcept {}

    T* allocate(std::size_t n) {
        if (n == 0)
            return nullptr;
        void* ptr = gHeap->alloc(n * sizeof(T));
        if (!ptr)
            HK_ABORT("Crashed while allocating for a vector!");
        return static_cast<T*>(ptr);
    }

    void deallocate(T* p, std::size_t n) noexcept {
        if (p)
            gHeap->free(p);
    }

    bool operator==(const PacketAllocator& other) const noexcept = default;
};

using PacketVector = std::vector<u8, PacketAllocator<u8>>;

struct Packet {
    virtual ~Packet() = default;
    virtual PacketType getType() = 0;
    virtual PacketVector serialize() = 0;
    virtual void deserialize(const PacketVector& data) = 0;

    nn::account::Uid mUserID;  // User ID of the packet owner
    PacketType mType = PacketType::UNKNOWN;
    short mPacketSize = 0;  // represents packet size without size of header
    bool mIsFail = false;
};

class PacketWriter {
public:
    PacketWriter(Packet* packet) : mPacket(packet) {
        packet->mType = packet->getType();
        writeToFullData(packet->mUserID.m_Storage[0]);
        writeToFullData(packet->mUserID.m_Storage[1]);
        writeToFullData(packet->getType());
    }

    template <typename T>
    void writeToFullData(T value) {
        const u8* ptr = reinterpret_cast<const u8*>(&value);
        mFullData.insert(mFullData.end(), ptr, ptr + sizeof(T));
    }

    template <typename T>
    void write(T value) {
        const u8* ptr = reinterpret_cast<const u8*>(&value);
        mData.insert(mData.end(), ptr, ptr + sizeof(T));
    }

    template <s32 L>
    void writeString(sead::FixedSafeString<L>& str) {
        s32 len = str.calcLength();
        mData.insert(mData.end(), str.getStringTop(), str.getStringTop() + len);

        if (len < L)
            mData.insert(mData.end(), L - len, 0);
    }

    PacketVector finalize() {
        u16 sizeNoHeader = mData.size();
        mPacket->mPacketSize = sizeNoHeader;
        writeToFullData(sizeNoHeader);
        mFullData.insert(mFullData.end(), mData.data(), mData.data() + mData.size());
        return std::move(mFullData);
    }

private:
    Packet* mPacket = nullptr;
    PacketVector mFullData;
    PacketVector mData;
    size mSizeOffset = 0;
};

class PacketReader {
public:
    PacketReader(Packet* packet, const u8* data, size size)
        : mPacket(packet), mData(data), mSize(size + sHeaderSize) {
        read(packet->mUserID.m_Storage[0]);
        read(packet->mUserID.m_Storage[1]);
        read(packet->mType);
        read(packet->mPacketSize);
    }

    template <typename T>
    void read(T& value) {
        if (mIsReadFail || mOffset + sizeof(T) > mSize) {
            hk::diag::logLine("Reading a value failed because it would go out of bounds. Offset: %zu Size: "
                              "%zu Type: %hd Type String: %s",
                              mOffset, mSize, mPacket->mType, packetNames[mPacket->mType]);
            mIsReadFail = true;
            return;
        }

        memcpy(&value, &mData[mOffset], sizeof(T));
        mOffset += sizeof(T);
    }

    template <s32 L>
    void readString(sead::FixedSafeString<L>& str) {
        if (mIsReadFail || mOffset + L > mSize) {
            hk::diag::logLine(
                "Reading a string failed because it would go out of bounds. Offset: %zu Size %zu", mOffset,
                mSize);
            mIsReadFail = true;
            str = "";
            return;
        }

        const char* strData = reinterpret_cast<const char*>(&mData[mOffset]);

        str.copy(strData, L);
        mOffset += L;
    }

    void finalize() { mPacket->mIsFail = mIsReadFail; }

private:
    Packet* mPacket = nullptr;
    const u8* mData = nullptr;
    size mSize = 0;
    size mOffset = 0;
    bool mIsReadFail = false;
};

// all packet types

// IWYU pragma: begin_keep
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
#include "packets/PacketFactory.h"
#include "packets/PacketHeader.h"
#include "packets/PlayerConnect.h"
#include "packets/PlayerDC.h"
#include "packets/PlayerInfPacket.h"
#include "packets/ShineCollect.h"
// IWYU pragma: end_keep