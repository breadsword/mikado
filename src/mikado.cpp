#include "mikado.h"

#include "utils.h"
#include "packets.h"

namespace mikado{

receiver_state receiver::state() const
{
    return m_state;
}


void receiver::put_byte(byte b)
{
    switch(m_state){
    case receiver_state::init:
        m_msg_type = b;
        m_state = receiver_state::got_type;
        break;

    case receiver_state::got_type:
        remaining_length = b;

        if (remaining_length == 0){
            m_state = receiver_state::msg_complete;
        }
        else
        {
            m_state = receiver_state::msg_incomplete;
        }
        break;

    case receiver_state::msg_incomplete:
        --remaining_length;
        if (remaining_length == 0){
            m_state = receiver_state::msg_complete;
        }
        break;

    case receiver_state::msg_complete:
    case receiver_state::error:
        // trying to parse after remaining length is processed
        // trying to parse after error
        m_state = receiver_state::error;
        break;
    }
}

void receiver::advance()
{
    if (end == std::end(buf))
    {
        // trying to read over end of buffer
        m_state = receiver_state::error;
        return;
    }

    const auto b = *end;
    ++end;

    put_byte(b);

}

void receiver::advance(size_t count)
{
    for(auto r=0; r<count; ++r)
    {
        advance();
    }
}


byte receiver::msg_type() const
{
    return m_msg_type;
}

gsl::span<const byte> receiver::content() const
{
    return gsl::make_span(start, end);
}

mikado_sm::mikado_sm(Connection& _conn, callback_t _cb) : conn{_conn}, cb{_cb}
{}

void mikado_sm::request_connect()
{
    const auto msg = connect::Packet{"client"}.to_span(conn.get_send_buf());
    conn.send(msg);
    m_state = state_t::connection_requested;
}

void mikado_sm::subscribe(const std::string topic)
{
    const auto msg = subscribe::Packet{1, topic}.to_span(conn.get_send_buf());
    conn.send(msg);
    m_state = state_t::subscribe_requested;
}

void mikado_sm::publish(gsl::span<const byte> topic, gsl::span<const byte> payload, bool retain)
{
    const auto msg = publish::Packet{topic, payload}.to_span(conn.get_send_buf());
    conn.send(msg);
}

void mikado_sm::process_packet(gsl::span<const byte> packet_buf)
{
    switch (m_state) {
    case state_t::connection_requested:
        process_packet_conn_requested(packet_buf);
        break;
    case state_t::subscribe_requested:
        process_packet_subscribe_requested(packet_buf);
        break;
    case state_t::connected:
        process_packet_connected(packet_buf);
        break;
    default:
        // no state where we expect a packet
        m_state = state_t::error;
        break;
    }

}

state_t mikado_sm::state() const
{
    return m_state;
}

// FIXME: this must go into packets.h
struct connack_packet
{
    bool session_present;
    uint8_t return_code;

    bool from_span(gsl::span<const byte> sp)
    {
        if (sp[0] != packet_type::connack)
        {
            // verify packet_type
            return false;
        }
        if (sp[1] != 2)
        {
            // verify remaining_length
            return false;
        }
        session_present = sp[2] & 0x1;
        return_code = sp[3];
        return true;
    }
};

void mikado_sm::process_packet_conn_requested(gsl::span<const byte> packet_buf)
{
    const auto packet_type = packet_buf[0];

    switch(packet_type)
    {
    case packet_type::connack:
    {
        auto p = connack_packet();
        const auto r = p.from_span(packet_buf);
        if (r && p.return_code==0)
        {
            m_state = state_t::connected;
        }
        else
        {
            m_state = state_t::error;
        }
    }
        break;

    default:
        m_state = state_t::error;
        break;
    }
}

void mikado_sm::process_packet_subscribe_requested(gsl::span<const byte> packet_buf)
{
    const auto packet_type = packet_buf[0];

    switch (packet_type)
    {
    case packet_type::suback:
    {
        suback::Packet p{};
        auto const r = p.from_span(packet_buf);
        if (r && p.is_valid() && p.result==suback::result_t::max_QoS_0)
        {
            m_state = state_t::connected;
        }
        else
        {
            m_state = state_t::error;
        }

    }
        break;
        //    case packet_type::publish:
        //    {

        //    }
    default:
        m_state = state_t::error;
        break;

    }
}

void mikado_sm::process_packet_connected(gsl::span<const byte> packet_buf)
{
    const auto packet_type = packet_buf[0];
    switch (packet_type) {
    case packet_type::publish:
    {
        publish::Packet p{};
        auto const r = p.from_span(packet_buf);
        if (r && p.is_valid())
        {
            cb(p.topic, p.payload);
        }
        m_state = state_t::connected;
    }
        break;

    default:
        m_state = state_t::error;
    }
}



}; // namespace mikado
