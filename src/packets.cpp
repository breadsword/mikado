#include <packets.h>

#include <algorithm>

#include "vbi.h"
#include "utils.h"

namespace mikado {

struct packet_stream
{
    typedef gsl::span<byte> buf_t;
    typedef gsl::span<const byte> cbuf_t;

    buf_t buf;
    buf_t::iterator cursor;

    packet_stream(const byte packet_header, buf_t _buf) :
        buf{_buf}, cursor{buf.begin() + 2}
    {
        buf[0] = packet_header;
    }

    buf_t content()
    {
        // add remaining_length info when returning content
        buf[1] = cursor - buf.begin() - 2;
        return gsl::make_span(buf.begin(), cursor);
    }

    // put byte in target
    packet_stream& operator<< (const mikado::byte value)
    {
        *cursor++ = value;
        return *this;
    }

    packet_stream& operator<< (const uint16_t value)
    {
        *cursor++ = msb(value);
        *cursor++ = lsb(value);
        return *this;
    }

    packet_stream& operator<< (const cbuf_t value)
    {
        const auto inserted = copy(value.begin(), value.end(),
                                   cursor, buf.end());
        cursor += inserted;
        return *this;
    }

    packet_stream& operator<< (const std::string& value)
    {
        const auto inserted = copy(value.begin(), value.end(),
                                   cursor, buf.end());
        cursor += inserted;
        return *this;
    }

};

gsl::span<byte> disconnect::Packet::to_span(gsl::span<byte> b)
{
    packet_stream s{packet_type::disconnect, b};
    return s.content();
}

} // namespace mikado


constexpr mikado::byte mikado::connect::Packet::protocol_name[];

mikado::connect::Packet::Packet(const std::string _clientID, const uint16_t _keep_alive, const mikado::byte _flags) : flags{_flags},
    keep_alive{_keep_alive}, clientID{_clientID}
{}

gsl::span<mikado::byte> mikado::connect::Packet::to_span(gsl::span<mikado::byte> buffer)
{
    packet_stream s{type, buffer};

    s << static_cast<uint16_t>(sizeof(protocol_name))
      << protocol_name
      << mqtt_protocol_version
      << flags
      << keep_alive
      << static_cast<uint16_t>(clientID.length())
      << clientID;

    return s.content();
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
    const uint8_t packet_head = (packet_type::subscribe | 0x2);
    packet_stream s{packet_head, d};

    s << packet_identifier
      << static_cast<uint16_t>(topic_filter.length())
      << topic_filter
      << QoS;

    return s.content();
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

gsl::span<mikado::byte> mikado::publish::Packet::to_span(gsl::span<mikado::byte> b)
{
    const uint8_t remaining_length = 2 + topic.size_bytes() + payload.size_bytes();

    const uint8_t first_byte = (packet_type::publish | QoS<<1 | retain);
    packet_stream s{first_byte, b};
    s << (uint16_t)topic.size_bytes()
      << topic
      << payload;
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
    const auto payload_start = &d[4] + topic_length;
    payload = gsl::make_span(payload_start, std::end(d));
    return true;
}


gsl::span<mikado::byte> mikado::pingreq::Packet::to_span(gsl::span<mikado::byte> d)
{
    packet_stream s{packet_type::pingreq, d};
    return s.content();
}

