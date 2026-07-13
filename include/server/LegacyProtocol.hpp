#pragma once

// The public SMOO+ 0.5 pre server serialises C# structs directly.  Do not use
// the C++ packet structs as a wire format here: their padding and several
// payloads differ from the server's layout.

#include <cmath>
#include <cstring>

#include <cstdint>

namespace LegacyProtocol {

using Byte = std::uint8_t;
using Size = std::int32_t;
using TypeId = std::int16_t;

constexpr Size HeaderSize = 20;
constexpr Size MaxPayloadSize = 0x100;
constexpr Size MaxFrameSize = HeaderSize + MaxPayloadSize;
constexpr const char* ServerVersion = "SMOO+ 0.5 pre";

enum Type : TypeId {
    Unknown = 0,
    Init = 1,
    Player = 2,
    Cap = 3,
    Game = 4,
    Tag = 5,
    Connect = 6,
    Disconnect = 7,
    Costume = 8,
    Shine = 9,
    Capture = 10,
    ChangeStage = 11,
    Command = 12,
    Chat = 13,
    UdpInit = 14,
    HolePunch = 15,
    Extra = 16,
    HealthCoins = 17,
    Mods = 18,
    ChangeCostume = 19,
};

struct Header {
    Byte userId[16];
    TypeId type;
    TypeId payloadSize;
};

static_assert(sizeof(Header) == HeaderSize, "Legacy TCP header must be 20 bytes");

inline bool getExpectedPayloadSize(TypeId type, TypeId* outSize) {
    TypeId size = -1;
    switch (type) {
    case Init: size = 34; break;
    case Player: size = 56; break;
    case Cap: size = 80; break;
    case Game: size = 66; break;
    case Tag: size = 5; break;
    case Connect: size = 38; break;
    case Disconnect: size = 0; break;
    case Costume: size = 64; break;
    case Shine: size = 4; break;
    case Capture: size = 32; break;
    case ChangeStage: size = 68; break;
    case Chat: size = 83; break;
    case Extra: size = 2; break;
    case HealthCoins: size = 5; break;
    case ChangeCostume: size = 64; break;
    default: return false;
    }

    if (outSize)
        *outSize = size;
    return true;
}

inline bool isValidHeader(const Header& header) {
    return header.payloadSize >= 0 && header.payloadSize <= MaxPayloadSize;
}

inline bool hasExpectedPayloadSize(const Header& header) {
    TypeId expected = 0;
    return !getExpectedPayloadSize(header.type, &expected) || expected == header.payloadSize;
}

inline bool decodeHeader(const Byte* bytes, Size size, Header* out) {
    if (!bytes || !out || size < HeaderSize)
        return false;
    std::memcpy(out, bytes, HeaderSize);
    return isValidHeader(*out);
}

inline void encodeHeader(Byte* bytes, const Byte* userId, TypeId type, TypeId payloadSize) {
    Header header{};
    if (userId)
        std::memcpy(header.userId, userId, sizeof(header.userId));
    header.type = type;
    header.payloadSize = payloadSize;
    std::memcpy(bytes, &header, HeaderSize);
}

template <typename T>
inline bool read(const Byte* bytes, Size size, Size offset, T* out) {
    if (!bytes || !out || offset < 0 || offset > size || static_cast<Size>(sizeof(T)) > size - offset)
        return false;
    std::memcpy(out, bytes + offset, sizeof(T));
    return true;
}

template <typename T>
inline bool write(Byte* bytes, Size size, Size offset, const T& value) {
    if (!bytes || offset < 0 || offset > size || static_cast<Size>(sizeof(T)) > size - offset)
        return false;
    std::memcpy(bytes + offset, &value, sizeof(T));
    return true;
}

inline bool copyFixedString(char* out, Size outSize, const Byte* bytes, Size size, Size offset, Size fieldSize) {
    if (!out || outSize <= 0 || !bytes || offset < 0 || fieldSize < 0 || offset > size || fieldSize > size - offset)
        return false;

    const Size copySize = fieldSize < outSize - 1 ? fieldSize : outSize - 1;
    std::memcpy(out, bytes + offset, copySize);
    out[copySize] = '\0';
    for (Size i = 0; i < copySize; i++) {
        if (out[i] == '\0')
            return true;
    }
    return true;
}

inline bool isFinite(float value) { return std::isfinite(value); }

inline bool isFiniteVec3(float x, float y, float z) {
    constexpr float MaxCoordinate = 1000000.0f;
    return isFinite(x) && isFinite(y) && isFinite(z) && std::fabs(x) <= MaxCoordinate &&
           std::fabs(y) <= MaxCoordinate && std::fabs(z) <= MaxCoordinate;
}

inline bool isNormalizedQuat(float x, float y, float z, float w) {
    if (!isFinite(x) || !isFinite(y) || !isFinite(z) || !isFinite(w))
        return false;
    const float lengthSquared = x * x + y * y + z * z + w * w;
    return lengthSquared >= 0.25f && lengthSquared <= 2.25f;
}

}  // namespace LegacyProtocol
