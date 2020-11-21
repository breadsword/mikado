#ifndef MIKADO_H_INCLUDED
#define MIKADO_H_INCLUDED

#include <gsl-lite/gsl-lite.hpp>

#include <packets.h>
#include <utils.h>
#include <vbi.h>

namespace mikado {

namespace gsl = ::gsl_lite; // convenience alias

enum class state_t{
    disconnected,
    connection_requested,
    connected,
    subscribe_requested,
    subscribed
};

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
