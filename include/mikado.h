#ifndef MIKADO_H_INCLUDED
#define MIKADO_H_INCLUDED

#include <gsl-lite/gsl-lite.hpp>

#include <packets.h>
#include <utils.h>

namespace mikado {

namespace gsl = ::gsl_lite; // convenience alias

template <class Connection>
class mikado{
public:
    mikado(Connection&);
    void connect();

    bool connected() const
    {return m_connected;}

private:
    Connection& conn;

    bool m_connected;
};

template <class Connection>
mikado<Connection>::mikado(Connection& _conn) : conn{_conn}, m_connected{false}
{

}

// TODO: use a toSpan() function of a Connect Packet to fill the buffer
// conn.send(connectPacket().toSpan(buffer))
gsl::span<byte>  build_connect_packet(gsl::span<byte> target_buffer);

template<class Connection>
void mikado<Connection>::connect()
{
    std::array<byte, 256> io_buffer {0};
    auto fixed_header = gsl::make_span(io_buffer.begin(), 2);
    auto remaining_buffer = gsl::make_span(&io_buffer[2], io_buffer.end());

    conn.send(build_connect_packet(io_buffer));

    // TODO: how many bytes?
    // idea: get all (for now); better: get 2 bytes, then remaining length
    const auto r = conn.recv(fixed_header);
    if (r < 0)
    {
        return;
    }
    const auto remaining_len = ::gsl::to_uchar(io_buffer[1]);

    conn.recv(remaining_buffer.first(remaining_len));

    const auto p = Packet::parse(io_buffer);
    // output can be:
    // - data corrupt
    // - packet complete, we can pick it up somewhere
    if (p->isValid())
    {
        // actually check content of parsed packet
        m_connected = true;
    }
}

}; // namespace mikado

#endif //MIKADO_H_INCLUDED
