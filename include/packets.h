#ifndef MIKADO_PACKETS_H_INCLUDED
#define MIKADO_PACKETS_H_INCLUDED

#include <gsl-lite/gsl-lite.hpp>
#include <utils.h>


namespace mikado {

namespace packet_type
{
constexpr byte connect   { 1 << 4};
constexpr byte connack   { 2 << 4};
constexpr byte publish   { 3 << 4};
constexpr byte subscribe { 8 << 4};
constexpr byte suback    { 9 << 4};
constexpr byte pingreq   {12 << 4};
constexpr byte pingresp  {13 << 4};
}; // namespace packet_type

namespace connect {

constexpr byte mqtt_protocol_version {4};

namespace flags {
constexpr byte username { 1 << 7};
constexpr byte password {1 << 6};
constexpr byte will_retain {1<< 5};

constexpr byte will_QoS(unsigned q)
{
    gsl_Expects(q<3);
    return gsl::narrow_cast<byte>(q << 3);
}

constexpr byte will_flag {1 << 2};
constexpr byte clean_start  {1 << 1};

} // namespace flags


struct Packet
{
public:
    Packet(const std::string _clientID="",
           const uint16_t _keep_alive = 0,
           const byte _flags = connect::flags::clean_start);
    gsl::span<byte> to_span(gsl::span<byte>);

    constexpr static auto type = packet_type::connect;

    constexpr static byte protocol_name[] {'M', 'Q', 'T', 'T'};
    byte flags;
    uint16_t keep_alive;
    const std::string clientID;
};

} // namespace connect

namespace connack {

enum class result_t{
    accepted = 0,
    refused_protocol_version = 1,
    refused_identifier_rejected = 2,
    refused_server_unavailable = 3,
    refused_authentication_failure = 4,
    refused_authorization_failure = 5
};

struct Packet
{
public:
    Packet();
    bool from_span(gsl::span<const byte>);

    bool session_present;
    result_t return_code;

};

} // namespace connack

namespace subscribe
{

struct Packet
{
    Packet(uint16_t _packet_identifier, const std::string& _topic_filter, byte _QoS=0);
    gsl::span<byte> to_span(gsl::span<byte>);

    uint16_t packet_identifier;
    std::string topic_filter;
    byte QoS{0};
};

} // namespace subscribe

namespace suback {

enum class result_t : byte
{
    max_QoS_0 = 0x00,
    max_QoS_1 = 0x01,
    max_QoS_2 = 0x02,
    failure = 0x80
};

struct Packet
{
    Packet();
    Packet(uint16_t _packet_identifier, result_t _result=result_t::max_QoS_0);

    uint16_t packet_identifier;
    result_t result;

    bool from_span(gsl::span<const byte>);
};

} // namespace suback

namespace publish {

struct Packet
{
    Packet();
    Packet(gsl::span<const byte> topic, gsl::span<const byte> payload,
           bool retain=false);
    gsl::span<byte> to_span(gsl::span<byte>);

    bool retain = false;
    uint8_t QoS = 0;
    gsl::span<const byte> topic;
    gsl::span<const byte> payload;

    bool from_span(gsl::span<const byte>);
};

} // namespace publish

namespace pingreq {

struct Packet
{
    gsl::span<byte> to_span(gsl::span<byte>);
};

} // namespace pingreq


} // namespace mikado

#endif //MIKADO_PACKETS_H_INCLUDED
