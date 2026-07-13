#include <array>
#include <cassert>
#include <cstring>
#include <limits>

#include "server/LegacyProtocol.hpp"

namespace {
void testHeaderAndFragmentedCopies() {
    std::array<LegacyProtocol::Byte, 16> id{};
    for (std::size_t i = 0; i < id.size(); i++)
        id[i] = static_cast<LegacyProtocol::Byte>(i);

    std::array<LegacyProtocol::Byte, LegacyProtocol::HeaderSize> wire{};
    LegacyProtocol::encodeHeader(wire.data(), id.data(), LegacyProtocol::Cap, 80);

    // TCP can split this header arbitrarily.  Reassemble the bytes before
    // decoding and verify the codec never relies on C++ packet casts.
    std::array<LegacyProtocol::Byte, LegacyProtocol::HeaderSize> reassembled{};
    std::memcpy(reassembled.data(), wire.data(), 7);
    std::memcpy(reassembled.data() + 7, wire.data() + 7, wire.size() - 7);

    LegacyProtocol::Header header{};
    assert(LegacyProtocol::decodeHeader(reassembled.data(), reassembled.size(), &header));
    assert(header.type == LegacyProtocol::Cap);
    assert(header.payloadSize == 80);
    assert(std::memcmp(header.userId, id.data(), id.size()) == 0);
}

void testExactLegacyLayoutTable() {
    struct Fixture {
        LegacyProtocol::TypeId type;
        LegacyProtocol::TypeId size;
    };
    constexpr Fixture fixtures[] = {
        {LegacyProtocol::Init, 34},       {LegacyProtocol::Player, 56},
        {LegacyProtocol::Cap, 80},         {LegacyProtocol::Game, 66},
        {LegacyProtocol::Tag, 5},          {LegacyProtocol::Connect, 38},
        {LegacyProtocol::Disconnect, 0},   {LegacyProtocol::Costume, 64},
        {LegacyProtocol::Shine, 4},        {LegacyProtocol::Capture, 32},
        {LegacyProtocol::ChangeStage, 68}, {LegacyProtocol::Chat, 83},
        {LegacyProtocol::Extra, 2},        {LegacyProtocol::HealthCoins, 5},
        {LegacyProtocol::ChangeCostume, 64},
    };

    for (const Fixture& fixture : fixtures) {
        LegacyProtocol::TypeId expected = 0;
        assert(LegacyProtocol::getExpectedPayloadSize(fixture.type, &expected));
        assert(expected == fixture.size);
    }

    LegacyProtocol::Header malformed{};
    malformed.type = LegacyProtocol::Game;
    malformed.payloadSize = 67;  // Client-only gameMode must not be read.
    assert(!LegacyProtocol::hasExpectedPayloadSize(malformed));
    malformed.type = LegacyProtocol::Shine;
    malformed.payloadSize = 5;  // Legacy Shine has no trailing boolean.
    assert(!LegacyProtocol::hasExpectedPayloadSize(malformed));

    // IDs 13-19 are server extensions, not the old client's coin/checkpoint/
    // moon/game-start packet meanings.
    assert(LegacyProtocol::Chat == 13);
    assert(LegacyProtocol::ChangeCostume == 19);
}

void testBoundsAndValidation() {
    LegacyProtocol::Header header{};
    header.payloadSize = LegacyProtocol::MaxPayloadSize + 1;
    assert(!LegacyProtocol::isValidHeader(header));

    std::array<LegacyProtocol::Byte, 4> small{};
    std::uint32_t value = 0;
    assert(!LegacyProtocol::read(small.data(), small.size(), 1, &value));
    const std::array<LegacyProtocol::Byte, 4> unterminated{'t', 'e', 's', 't'};
    char terminated[4] = {};
    assert(LegacyProtocol::copyFixedString(terminated, sizeof(terminated), unterminated.data(),
                                           unterminated.size(), 0, unterminated.size()));
    assert(terminated[3] == '\0');
    assert(!LegacyProtocol::isFiniteVec3(0.0f, 0.0f, std::numeric_limits<float>::quiet_NaN()));
    assert(!LegacyProtocol::isNormalizedQuat(0.0f, 0.0f, 0.0f, 0.0f));
    assert(LegacyProtocol::isNormalizedQuat(0.0f, 0.0f, 0.0f, 1.0f));
}
}  // namespace

int main() {
    testHeaderAndFragmentedCopies();
    testExactLegacyLayoutTable();
    testBoundsAndValidation();
    return 0;
}
