#include <cstddef>
#include <cstdint>

#include "server/LegacyProtocol.hpp"

// Build with -DLEGACY_PROTOCOL_ENABLE_FUZZER=ON under Clang.  This exercises
// the byte-only parser with ASan/UBSan before a target build ever sees a frame.
extern "C" int LLVMFuzzerTestOneInput(const std::uint8_t* data, std::size_t size) {
    if (size > static_cast<std::size_t>(LegacyProtocol::MaxFrameSize))
        return 0;
    LegacyProtocol::Header header{};
    if (!LegacyProtocol::decodeHeader(data, static_cast<LegacyProtocol::Size>(size), &header))
        return 0;

    if (!LegacyProtocol::hasExpectedPayloadSize(header))
        return 0;
    if (size < static_cast<std::size_t>(LegacyProtocol::HeaderSize + header.payloadSize))
        return 0;

    const LegacyProtocol::Byte* payload = data + LegacyProtocol::HeaderSize;
    char fixedString[65] = {};
    LegacyProtocol::copyFixedString(fixedString, sizeof(fixedString), payload, header.payloadSize, 0,
                                    header.payloadSize);
    return 0;
}
