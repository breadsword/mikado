#include <packets.h>

#include <algorithm>

#include "utils.h"

gsl::span<mikado::byte> mikado::connect::Packet::toSpan(gsl::span<mikado::byte> buffer)
{
    // fixed header
    buffer[0] = type;

    // beginning of variable header
    auto head = buffer.begin() + 2;

    // TODO: check that buffer is large enough
    // constexpr static auto p_span = gsl::make_span(protocol_name_ver);
    auto it = std::copy(std::begin(protocol_name_ver), std::end(protocol_name_ver), head);

    *it++ = flags;

    *it++ = msb(keep_alive);
    *it++ = lsb(keep_alive);

    *it++ = byte{0}; // Property length

    // Payload
    it = std::copy(clientID.begin(), clientID.end(), it);

    // calculate length
    const auto remaining_length = it-head;
    // it = std::copy(vbi_encoder(remaining_length), nullptr, it);

    return {buffer.begin(), it};
}
