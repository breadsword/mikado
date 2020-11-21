#include <packets.h>

#include <algorithm>

#include "vbi.h"
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
    return valid;
}

std::unique_ptr<mikado::Packet> mikado::Packet::parse(
        byte packet_type,
        gsl::span<mikado::byte> packet_data)
{
    std::unique_ptr<Packet> res;
    switch (packet_type){
    case packet_type::connack:
        res = std::make_unique<connack::Packet>();
        break;
    }
    res->valid = res->from_span(packet_data);

    return res;
}

mikado::connack::Packet::~Packet()
{
}

mikado::connack::Packet::Packet()
{
}

gsl::span<mikado::byte> mikado::connack::Packet::to_span(gsl::span<mikado::byte> buf)
{
    return buf.first(0);
}

bool mikado::connack::Packet::from_span(gsl::span<mikado::byte> d)
{
    const auto connect_acknowledge_flags = d[0];
    if (connect_acknowledge_flags == 0)
        session_present = 0;
    else if (connect_acknowledge_flags == 1)
        session_present = 1;
    else
        return false;

    reason_code = d[1];
    if (reason_code == 0)
    {
        connected = true;
    }

    vbi_decoder prop_len{};
    auto cursor = d.begin() + 2;
    while (prop_len)
    {
        prop_len.read_byte(*cursor);
        cursor++;
    }

    gsl::span<byte> prop_span =
            gsl::make_span(cursor, vbi_decoder::value_type(prop_len));

    // while (cursor != d.end())
    const byte prop_type = *cursor++;
    switch (static_cast<prop>(prop_type)) {
    case prop::session_expiry:
        session_expiry = *cursor++;
        break;
    case prop::receive_maximum:
    {
        const auto m_msb = *cursor++;
        const auto m_lsb = *cursor++;
        receive_maximum =  (m_msb<<8) + m_lsb;
    }
        break;
    default:
        return false;
    }

    return false;
}

