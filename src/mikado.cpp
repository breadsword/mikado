#include "mikado.h"

#include "utils.h"
#include "packets.h"

namespace mikado{


gsl::span<byte> build_connect_packet(gsl::span<byte> target_buffer)
{
    // FIXME: we need to ensure the packet fits into target_buffer!

    target_buffer[0] = packet_type::connect;

    auto it = target_buffer.begin() + 1;
    *(++it) = byte{0};
    *(++it) = byte{4};
    *(++it) = byte{'M'};
    *(++it) = byte{'Q'};
    *(++it) = byte{'T'};
    *(++it) = byte{'T'};
    *(++it) = connect::mqtt_protocol_version;

    *(++it) = connect::flags::clean_start | connect::flags::will_QoS(0);

    constexpr uint16_t keep_alive = 0;
    *(++it) = msb(keep_alive);
    *(++it) = lsb(keep_alive);
    ++it;

    constexpr uint16_t property_length = 0;
    it = std::copy(vbi(property_length).begin(), vbi::end(), it);

    // attach a clientID
    const std::string clientID{"client"};
    it = std::copy(vbi(clientID.length()).begin(), vbi::end(), it);
    it = std::copy(clientID.begin(), clientID.end(), it);

    const auto remaining_length = it - target_buffer.begin() - 2;
    target_buffer[1] = remaining_length;

    return {target_buffer.begin(), it};
}

std::unique_ptr<Packet> Packet::parse(gsl::span<byte> data)
{
    return nullptr;
}

}
