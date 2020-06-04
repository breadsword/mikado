#include "mikado.h"

namespace mikado{

constexpr auto mqtt_protocol_version = 5;

namespace connect_flags {
constexpr auto username = 1 << 7;
constexpr auto password = 1 << 6;
constexpr auto will_retain = 1<< 5;

constexpr auto will_QoS(unsigned q)
{
    gsl_Expects(q<3);
    return q << 3;
}

constexpr auto will_flag = 1 << 2;
constexpr auto clean_start = 1 << 1;
}

constexpr unsigned char msb(uint16_t n)
{
    return n >> 8;
}

constexpr unsigned char lsb(uint16_t n)
{
    return n & (0xFF);
}

gsl::span<unsigned char> build_connect_packet(gsl::span<unsigned char> target_buffer)
{
    target_buffer[0] = packet_type::CONNECT;

    auto it = target_buffer.begin() + 1;
    *(++it) = 0;
    *(++it) = 4;
    *(++it) = 'M';
    *(++it) = 'Q';
    *(++it) = 'T';
    *(++it) = 'T';
    *(++it) = mqtt_protocol_version;

    *(++it) = connect_flags::clean_start | connect_flags::will_QoS(0);

    constexpr uint16_t keep_alive = 0;
    *(++it) = msb(keep_alive);
    *(++it) = lsb(keep_alive);

    constexpr uint16_t property_length = 0;
    // FIXME: we should encode property_length as variable length integer here
    *(++it) = lsb(property_length);

    // attach a clientID
    const std::string clientID{"client"};
    // FIXME: we should encode property_length as variable length integer here
    *(++it) = lsb(clientID.length());
    it = std::copy(clientID.begin(), clientID.end(), ++it);

    const auto remaining_length = it - target_buffer.begin() - 2;
    target_buffer[1] = remaining_length;

    return {target_buffer.begin(), it};
}

std::unique_ptr<Packet> Packet::parse(gsl::span<unsigned char> data)
{
    return nullptr;
}

}
