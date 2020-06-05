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

    static std::unique_ptr<Packet> parse(gsl::span<byte> data);

protected:
    virtual bool from_span(gsl::span<byte>) = 0;
};

namespace packet_type
{
constexpr byte connect  {1 << 4};

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
    virtual ~Packet();
    Packet(const std::string _clientID="",
           const uint16_t _keep_alive = 0,
           const byte _flags = connect::flags::clean_start) : flags{_flags}, keep_alive{_keep_alive}, clientID{_clientID}
    {}

    constexpr static auto type = packet_type::connect;

//    constexpr static byte protocol_name[] =
//    {0, 4, 'M', 'Q', 'T', 'T'};

    constexpr static byte protocol_name[] {'M', 'Q', 'T', 'T'};

    byte flags;
    uint16_t keep_alive;
    const std::string clientID;

    virtual gsl::span<byte> to_span(gsl::span<byte>) override;
protected:
    virtual bool from_span(gsl::span<byte>) override;
};

} // namespace connect



} // namespace mikado

#endif //MIKADO_PACKETS_H_INCLUDED
