#ifndef MIKADO_PACKETS_H_INCLUDED
#define MIKADO_PACKETS_H_INCLUDED

#include <gsl-lite/gsl-lite.hpp>
#include <utils.h>


namespace mikado {

class Packet
{
public:
    virtual ~Packet();

    virtual bool is_valid();
    virtual gsl::span<byte> to_span(gsl::span<byte>) = 0;

    /// Require, that the packet is completely (including fixed header) inside data
    static std::unique_ptr<Packet> parse(byte packet_type, gsl::span<byte> packet_data);

protected:
    virtual bool from_span(gsl::span<byte>) = 0;
    bool valid = false;
};

namespace packet_type
{
constexpr byte connect  {1 << 4};
constexpr byte connack  {2 << 4};

}; // namespace packet_type

namespace connect {

constexpr byte mqtt_protocol_version {5};

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


struct Packet : public ::mikado::Packet
{
public:
    virtual ~Packet();
    Packet(const std::string _clientID="",
           const uint16_t _keep_alive = 0,
           const byte _flags = connect::flags::clean_start) : flags{_flags}, keep_alive{_keep_alive}, clientID{_clientID}
    {}
    virtual gsl::span<byte> to_span(gsl::span<byte>) override;

    constexpr static auto type = packet_type::connect;

    constexpr static byte protocol_name[] {'M', 'Q', 'T', 'T'};
    byte flags;
    uint16_t keep_alive;
    const std::string clientID;

protected:
    virtual bool from_span(gsl::span<byte>) override;
};

} // namespace connect

namespace connack {

struct Packet : public ::mikado::Packet
{
public:
    virtual ~Packet();
    Packet();
    virtual gsl::span<byte> to_span(gsl::span<byte>) override;

    constexpr static auto type = packet_type::connack;

    bool session_present;
    byte reason_code;
    bool connected = false;

    int session_expiry = -1;
    int receive_maximum = -1;
    int maximum_qos = -1;
    int retain_available = -1;
    std::string client_identifier = "";
    int server_keepalive = -1;

protected:
    virtual bool from_span(gsl::span<byte>) override;
};

enum class prop : byte
{
    session_expiry = 0x11,
    receive_maximum = 0x21,
    maximum_qos = 0x24
};

} // namespace connack

} // namespace mikado

#endif //MIKADO_PACKETS_H_INCLUDED
