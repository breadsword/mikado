#ifndef MIKADO_PACKETS_H_INCLUDED
#define MIKADO_PACKETS_H_INCLUDED

#include <gsl-lite/gsl-lite.hpp>
#include <utils.h>


namespace mikado {

class Packet
{
public:
    virtual ~Packet();

    virtual bool isValid()
    {
        return false;
    }
    virtual gsl::span<byte> toSpan(gsl::span<byte>) = 0;

    static std::unique_ptr<Packet> parse(gsl::span<byte> data);

private:
    virtual bool fromSpan(gsl::span<byte>) = 0;
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
    constexpr static auto type = packet_type::connect;

    constexpr static byte protocol_name_ver[] =
    {0, 4, 'M', 'Q', 'T', 'T', mqtt_protocol_version};

    byte flags = connect::flags::clean_start;
    uint16_t keep_alive = 0;

    const std::string clientID = "";

    virtual gsl::span<byte> toSpan(gsl::span<byte>) override;

};

} // namespace connect



} // namespace mikado

#endif //MIKADO_PACKETS_H_INCLUDED
