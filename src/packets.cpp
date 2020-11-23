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

bool mikado::connect::Packet::from_span(gsl::span<const mikado::byte>)
{
    return false;
}

mikado::Packet::~Packet()
{

}

std::unique_ptr<mikado::Packet> mikado::Packet::parse(
        byte packet_type,
        gsl::span<const mikado::byte> packet_data)
{
    std::unique_ptr<Packet> res;
    switch (packet_type){
    case packet_type::connack:
        res = std::make_unique<connack::Packet>();
        break;
    }
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


mikado::subscribe::Packet::~Packet()
{}

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

bool mikado::subscribe::Packet::from_span(gsl::span<const mikado::byte> d)
{
    if (d[0] != (packet_type::connack | 0x2))
    {
        return false;
    }

    uint8_t remaining_length = d[1];
    packet_identifier = (d[2]<<8) + d[3];

    uint16_t payload_length = (d[4] << 8) + d[5];

    topic_filter = std::string (&d[6], &d[6]+payload_length);
    if (d[6+payload_length] != 0)
    {
        return false;
    }
    return true;
}

mikado::suback::Packet::~Packet()
{

}

mikado::suback::Packet::Packet()
{

}

gsl::span<mikado::byte> mikado::suback::Packet::to_span(gsl::span<mikado::byte>b)
{
    return gsl::make_span(std::begin(b), 0);
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

mikado::publish::Packet::~Packet()
{}

mikado::publish::Packet::Packet()
{}

mikado::publish::Packet::Packet(gsl::span<const mikado::byte> _topic,
                                gsl::span<const mikado::byte> _payload,
                                bool _retain) :
    topic{_topic}, payload{_payload}, retain{_retain}
{
}

gsl::span<mikado::byte> mikado::publish::Packet::to_span(gsl::span<mikado::byte> b)
{
    auto it = std::begin(b);

    *it++ = packet_type::publish | QoS<<1 | retain;
    const uint16_t remaining_length = 2 + topic.size_bytes() + payload.size_bytes();

    *it++ = remaining_length;

    *it++ = msb(topic.size_bytes());
    *it++ = lsb(topic.size_bytes());
    const auto inserted = copy(topic.begin(), topic.end(), it, std::end(b));
    it+=inserted;

    const auto inserted2 = copy(payload.begin(), payload.end(), it, std::end(b));
    it +=inserted2;

    return gsl::make_span(std::begin(b), it);
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

mikado::pingreq::Packet::~Packet()
{}

mikado::pingreq::Packet::Packet()
{}

gsl::span<mikado::byte> mikado::pingreq::Packet::to_span(gsl::span<mikado::byte> d)
{
    d[0] = packet_type::pingreq;
    d[1] = 0;
    return gsl::make_span(std::begin(d), 2);
}

bool mikado::pingreq::Packet::from_span(gsl::span<const mikado::byte>)
{
    return false;
}
