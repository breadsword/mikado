#ifndef MIKADO_H_INCLUDED
#define MIKADO_H_INCLUDED

#include <gsl-lite/gsl-lite.hpp>

#include <packets.h>
#include <utils.h>
#include <vbi.h>

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

template<class Connection>
void mikado<Connection>::connect()
{
    std::array<byte, 256> io_buffer {0};

    conn.send(connect::Packet{"client"}.to_span(io_buffer));

    // get first byte -> packet type
    const auto r = conn.recv(gsl::make_span(io_buffer.begin(), 1));
    if (r < 0)
    {
        return;
    }
    // now decode remaining length as vbi
    vbi_decoder d_rem_len{};
    auto no_digits = 0;
    while (d_rem_len){
        byte digit[1] = {0};
        conn.recv(digit);
        d_rem_len.read_byte(digit[0]);
        no_digits++;
    }

    const auto remaining_len = vbi_decoder::value_type(d_rem_len);
    auto packet_data = gsl::make_span(&io_buffer[no_digits+1], io_buffer.end());

    conn.recv(packet_data.first(remaining_len));

    const auto connack = Packet::parse(io_buffer[0], packet_data);
    // output can be:
    // - data corrupt
    // - packet complete, we can pick it up somewhere
    if (connack && connack->is_valid())
    {
        // actually check content of parsed packet
        m_connected = true;
    }
}

}; // namespace mikado

#endif //MIKADO_H_INCLUDED
