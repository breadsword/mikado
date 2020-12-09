#include "mikado.h"

#include "utils.h"
#include "packets.h"

namespace mikado
{

receiver_state receiver::state() const
{
    return m_state;
}

void receiver::consume_byte(byte b)
{
    switch (m_state)
    {
    case receiver_state::init:
        m_state = receiver_state::got_type;
        break;

    case receiver_state::got_type:
        // adjust how far to read
        read_until = start + 2 + b;

        if (cursor == read_until)
        {
            m_state = receiver_state::msg_complete;
        }
        else
        {
            m_state = receiver_state::msg_incomplete;
        }
        break;

    case receiver_state::msg_incomplete:
        if (cursor == read_until)
        {
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
    if (cursor == buf.end())
    {
        // trying to read over end of buffer
        m_state = receiver_state::error;
        return;
    }

    const auto b = *cursor;
    ++cursor;

    consume_byte(b);
}

void receiver::advance(size_t count)
{
    for (auto r = 0; r < count; ++r)
    {
        advance();
    }
}

void receiver::advance_until(cbuf_t::iterator target)
{
    while( (cursor < target) && (m_state != receiver_state::error) )
    {
        advance();
    }
}

byte receiver::msg_type() const
{
    return *start;
}

void receiver::reset()
{
    m_state = receiver_state::init;
    cursor = start;
    read_until = start +2;
}

ptrdiff_t receiver::bytes_to_read() const
{
    return read_until - cursor;
}

mikado::receiver::operator bool() const
{
    return ((state() != receiver_state::error) &&
            (state() != receiver_state::msg_complete));
}

gsl::span<const byte> receiver::content() const
{
    return gsl::make_span(start, cursor);
}

mikado_sm::mikado_sm(Connection &_conn, callback_t _cb) : conn(_conn), cb{_cb}
{
}

void mikado_sm::request_connect(const std::string &client)
{
    const auto msg = connect::Packet{client}.to_span(conn.get_send_buf());
    conn.send(msg);
    m_state = state_t::connection_requested;
}

void mikado_sm::subscribe(const std::string topic)
{
    const auto msg = subscribe::Packet{(5 << 8) + 9, topic}.to_span(conn.get_send_buf());
    conn.send(msg);
    m_state = state_t::subscribe_requested;
}

void mikado_sm::publish(const std::string &topic, const std::string &payload,
                        bool retain)
{
    publish(cbuf_t(reinterpret_cast<const byte *>(topic.data()), topic.length()),
            cbuf_t(reinterpret_cast<const byte *>(payload.data()), payload.length())
            );
}

void mikado_sm::publish(gsl::span<const byte> topic, gsl::span<const byte> payload, bool retain)
{
    const auto msg = publish::Packet{topic, payload}.to_span(conn.get_send_buf());
    conn.send(msg);
}

void mikado_sm::process_packet(gsl::span<const byte> packet_buf)
{
    switch (m_state)
    {
    case state_t::connection_requested:
        process_packet_conn_requested(packet_buf);
        break;
    case state_t::subscribe_requested:
        process_packet_subscribe_requested(packet_buf);
        break;
    case state_t::connected:
        process_packet_connected(packet_buf);
        break;
    case state_t::ping_await:
        process_packet_ping_await(packet_buf);
        break;
    default:
        // no state where we expect a packet
        m_state = state_t::error;
        break;
    }
}

void mikado_sm::send_ping()
{
    const auto msg = pingreq::Packet{}.to_span(conn.get_send_buf());
    conn.send(msg);
    m_state = state_t::ping_await;
}

void mikado_sm::send_disconnect()
{
    const auto msg = disconnect::Packet{}.to_span(conn.get_send_buf());
    conn.send(msg);
    m_state = state_t::disconnected;
}

void mikado_sm::reset()
{
    m_state = state_t::disconnected;
}

state_t mikado_sm::state() const
{
    return m_state;
}

void mikado_sm::process_packet_conn_requested(gsl::span<const byte> packet_buf)
{
    const auto packet_type = packet_buf[0];

    switch (packet_type)
    {
    case packet_type::connack:
    {
        auto p = connack::Packet{};
        const auto r = p.from_span(packet_buf);
        if (r && p.return_code == connack::result_t::accepted)
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
        if (r && p.result == suback::result_t::max_QoS_0)
        {
            m_state = state_t::connected;
        }
        else
        {
            m_state = state_t::error;
        }
    }
        break;
    case packet_type::publish:
    {
        if (handle_publish(packet_buf))
        {
        }
        else
        {
            m_state = state_t::error;
        }
    }
    default:
        m_state = state_t::error;
        break;
    }
}

bool mikado::mikado_sm::handle_publish(gsl::span<const byte> packet_buf)
{
    publish::Packet p{};
    auto const r = p.from_span(packet_buf);
    if (r)
    {
        cb(p.topic, p.payload);
        return true;
    }
    return false;
}

void mikado_sm::process_packet_connected(gsl::span<const byte> packet_buf)
{
    const auto packet_type = packet_buf[0];
    switch (packet_type)
    {
    case packet_type::publish:
    {
        if (handle_publish(packet_buf))
        {
        }
        else
        {
            m_state = state_t::error;
        }
    }
        break;

    default:
        m_state = state_t::error;
    }
}

void mikado_sm::process_packet_ping_await(gsl::span<const byte> packet_buf)
{
    const auto packet_type = packet_buf[0];
    switch (packet_type)
    {
    case packet_type::publish:
    {
        if (handle_publish(packet_buf))
        {
        }
        else
        {
            m_state = state_t::error;
        }
    }
        break;
    case packet_type::pingresp:
    {
        if (packet_buf[1] == 0)
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

read_result Packet_reader::read_packet()
{
    const auto r = conn.read(
                buf_t{cursor, static_cast<unsigned long>(rec.bytes_to_read())});
    if (r > 0)
    {
        cursor += r;
        rec.advance_until(cursor);
    }
#ifdef errno
    else if ((r < 0) && (errno == EAGAIN))
    {
        return read_result::more_to_read;
    }
#endif
    else
    {
        return read_result::read_error;
    }

    if (rec.state() == receiver_state::msg_complete)
    {
        cursor = read_buffer.begin();
        return read_result::success;
    }
    else
    {
        return read_result::more_to_read;
    }

}

cbuf_t Packet_reader::content() const
{
    return rec.content();
}

void Packet_reader::reset()
{
    cursor = read_buffer.begin();
    rec.reset();
}

}; // namespace mikado
