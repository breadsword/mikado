#include <packets.h>

#include <algorithm>

#include "vbi.h"
#include "utils.h"

constexpr mikado::byte mikado::connect::Packet::protocol_name[];

mikado::connect::Packet::Packet(const std::string _clientID, const uint16_t _keep_alive, const mikado::byte _flags) : flags{_flags},
    keep_alive{_keep_alive}, clientID{_clientID}
{}

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



mikado::connack::Packet::Packet()
{
}

bool mikado::connack::Packet::from_span(gsl::span<const mikado::byte> d)
{
    if (d[0] != packet_type::connack)
    {
        // verify packet_type
        return false;
    }
    if (d[1] != 2)
    {
        // verify remaining_length
        return false;
    }
    session_present = d[2] & 0x1;

    if (d[3] >=0 && d[3] < 6)
    {
        return_code = static_cast<result_t>(d[3]);
    }
    else
    {
        return false;
    }

    return true;
}


mikado::subscribe::Packet::Packet(uint16_t _packet_identifier,
                                  const std::string &_topic_filter,
                                  mikado::byte _QoS) :
    packet_identifier{_packet_identifier}, topic_filter{_topic_filter}, QoS{_QoS}
{}

gsl::span<mikado::byte> mikado::subscribe::Packet::to_span(gsl::span<mikado::byte> d)
{
    auto it = &d[0];
    *it++ = (packet_type::subscribe | 0x2);
    // fill in remaining_length later (?)
    const auto remaining_length = 2+2+topic_filter.length() + 1;
    *it++ = remaining_length;
    *it++ = msb(packet_identifier);
    *it++ = lsb(packet_identifier);
    *it++ = msb(topic_filter.length());
    *it++ = lsb(topic_filter.length());

    const auto inserted = copy(topic_filter.begin(), topic_filter.end(), it, std::end(d)-1);
    it +=inserted;
    *it++ = 0;

    return gsl::make_span(std::begin(d), it);
}

mikado::suback::Packet::Packet()
{
}

bool mikado::suback::Packet::from_span(gsl::span<const mikado::byte> d)
{
    if (d[0] != packet_type::suback)
    {
        return false;
    }
    uint8_t remaining_length = d[1];
    if (remaining_length != 3)
    {
        return false;
    }
    packet_identifier = d[2] * 256 + d[3];

    // we may be passed any byte value, so after conversion to enum class
    // I want to make sure the value is actually a defined enum value.
    if ((d[4] >=0 && d[4] <=2) || d[4] == 0x80)
    {
        result = static_cast<result_t>(d[4]);
    }
    else
    {
        return false;
    }

    return true;
}

mikado::publish::Packet::Packet()
{}

mikado::publish::Packet::Packet(gsl::span<const mikado::byte> _topic,
                                gsl::span<const mikado::byte> _payload,
                                bool _retain) :
    topic{_topic}, payload{_payload}, retain{_retain}
{}

// put byte in target
auto operator<< (gsl::span<mikado::byte> target, const mikado::byte value)
{
    target[0] = value;
    return target.subspan(1);
}

// put uint16_t in target
auto operator<< (gsl::span<mikado::byte> target, const uint16_t value)
{
    target[0] = mikado::msb(value);
    target[1] = mikado::lsb(value);

    return target.subspan(2);
}

// put string in target
auto operator<<(gsl::span<mikado::byte> target, gsl::span<const mikado::byte> value)
{
    auto inserted = mikado::copy(value.begin(), value.end(), target.begin(), target.end());
    return target.subspan(inserted);
}

namespace mikado {

struct span_stream
{
    typedef gsl::span<byte> buf_t;
    typedef gsl::span<const byte> cbuf_t;

    span_stream(buf_t _buf) : buf{_buf}, cursor{buf.begin()} {}
    span_stream(buf_t _buf, buf_t::iterator _cursor) : buf{_buf}, cursor{_cursor} {}

    buf_t buf;
    buf_t::iterator cursor = buf.begin();
    buf_t::iterator begin = buf.begin();


    buf_t content()
    {
        return gsl::make_span(buf.begin(), cursor);
    }

    // put byte in target
    span_stream& operator<< (const mikado::byte value)
    {
        *cursor++ = value;
        return *this;
    }

    span_stream& operator<< (const uint16_t value)
    {
        *cursor++ = msb(value);
        *cursor++ = lsb(value);
        return *this;
    }

    span_stream& operator<< (const cbuf_t value)
    {
        const auto inserted = copy(value.begin(), value.end(),
                                   cursor, buf.end());
        cursor += inserted;
        return *this;
    }

};

} // namespace mikado

gsl::span<mikado::byte> mikado::publish::Packet::to_span(gsl::span<mikado::byte> b)
{
    const uint8_t remaining_length = 2 + topic.size_bytes() + payload.size_bytes();

    span_stream s{b};
    s << (uint8_t)(packet_type::publish | QoS<<1 | retain)
      << remaining_length
      << (uint16_t)topic.size_bytes()
      << topic
      << payload
         ;
    return s.content();
}

bool mikado::publish::Packet::from_span(gsl::span<const mikado::byte> d)
{
    if ((d[0] & 0xF0) != packet_type::publish)
    {
        return false;
    }
    retain = (d[0] & 0x01);
    QoS = (d[0] & 0x6);
    if (QoS != 0)
    {
        // This parser does not support QoS 1 or 2 and will not deal with a
        // packet identifier
        return false;
    }
    uint8_t remaining_length = d[1];

    uint16_t topic_length = d[2]*256 + d[3];
    topic = gsl::make_span(&d[4], topic_length);
    const auto payload_start = &d[topic_length+4];
    payload = gsl::make_span(payload_start, std::end(d));
    return true;
}


gsl::span<mikado::byte> mikado::pingreq::Packet::to_span(gsl::span<mikado::byte> d)
{
    d[0] = packet_type::pingreq;
    d[1] = 0;
    return gsl::make_span(std::begin(d), 2);
}

