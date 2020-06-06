#include <packets.h>

#include <algorithm>

#include "utils.h"

mikado::connect::Packet::~Packet()
{

}

constexpr mikado::byte mikado::connect::Packet::protocol_name[];

gsl::span<mikado::byte> mikado::connect::Packet::to_span(gsl::span<mikado::byte> buffer)
{
    // fixed header
    buffer[0] = type;

    // beginning of buffer (for debugging)
    const auto start = buffer.begin();
    // beginning of variable header
    const auto head = buffer.begin() + 2;

    // TODO: check that buffer is large enough

    auto it = head;
    *it++ = msb(sizeof(protocol_name));
    *it++ = lsb(sizeof(protocol_name));
    it = std::copy(std::begin(protocol_name), std::end(protocol_name), it);
    *it++ = mqtt_protocol_version;

    *it++ = flags;

    *it++ = msb(keep_alive);
    *it++ = lsb(keep_alive);

    constexpr uint16_t property_length = 0;
    it = std::copy(vbi(property_length).begin(), vbi::end(), it);

    // Payload
    it = std::copy(vbi(clientID.length()).begin(), vbi::end(), it);
    it = std::copy(clientID.begin(), clientID.end(), it);

    // calculate length
    const auto remaining_length = it-head;
    //FIXME: this will overwrite stuff if we have more than one digit
    std::copy(vbi(remaining_length).begin(), vbi::end(), start+1);

    return {buffer.begin(), it};
}

bool mikado::connect::Packet::from_span(gsl::span<mikado::byte>)
{
    return false;
}

mikado::Packet::~Packet()
{

}

bool mikado::Packet::is_valid()
{
    return false;
}

std::unique_ptr<mikado::Packet> mikado::Packet::parse(gsl::span<mikado::byte> data)
{
    return nullptr;
}
