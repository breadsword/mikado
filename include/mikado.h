#ifndef MIKADO_H_INCLUDED
#define MIKADO_H_INCLUDED

#include <gsl-lite/gsl-lite.hpp>

#include <packets.h>
#include <utils.h>
#include <vbi.h>

#include <vector>

namespace mikado {

namespace gsl = ::gsl_lite; // convenience alias

enum class state_t{
    disconnected,
    connection_requested,
    connected,
    subscribe_requested,
    subscribed
};

enum class receiver_state{
    init,
    got_type,
    msg_incomplete,
    msg_complete,
    error
};

class receiver{
public:
    //TODO: make this non-copying by keeping two cursors into a buffer instead of vector

    // buf is the complete range we could expect to read a message from
    // start and end are cursors pointing to the start and end of the parsed
    // packet
    // We do not keep a separate marker for the beginning of the mqtt variable header,
    // as it is always start+2.
    // payload start (as beginning of packet payload) belongs into packet parser.
    receiver(gsl::span<const byte> read_buffer) :
        buf{read_buffer}, start{std::begin(read_buffer)}, end{start}
    {}
    receiver_state state() const;

    void put_byte(byte b);

    void advance();
    void advance(size_t count);

    byte msg_type() const;

    gsl::span<const byte> content() const;

private:
    receiver_state m_state = receiver_state::init;

    byte m_msg_type;
    uint8_t remaining_length;

    gsl::span<const byte> buf;
    const byte *start, *end=nullptr;

    std::vector<byte> packet;

}; // class receiver

template <class Connection>
class mikado{
public:
    mikado(Connection&);
    void request_connect();

    state_t state() const
    {
        return m_state;
    }

private:
    Connection& conn;

    state_t m_state = state_t::disconnected;
};

template <class Connection>
mikado<Connection>::mikado(Connection& _conn) : conn{_conn}
{
}

template<class Connection>
void mikado<Connection>::request_connect()
{
    std::array<byte, 256> io_buffer {0};

    conn.send(connect::Packet{"client"}.to_span(io_buffer));

    m_state = state_t::connection_requested;
}

}; // namespace mikado

#endif //MIKADO_H_INCLUDED
