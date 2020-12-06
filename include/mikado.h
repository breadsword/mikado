#ifndef MIKADO_H_INCLUDED
#define MIKADO_H_INCLUDED

#include <gsl-lite/gsl-lite.hpp>

#include <packets.h>
#include <utils.h>
#include <vbi.h>

namespace mikado
{

typedef gsl::span<const byte> cbuf_t;
typedef gsl::span<byte> buf_t;


enum class state_t
{
    disconnected,
    connection_requested,
    connected,
    subscribe_requested,
    ping_await,
    error
};

enum class receiver_state
{
    init,
    got_type,
    msg_incomplete,
    msg_complete,
    error
};

class receiver
{
public:

    // buf is the complete range we could expect to read a message from
    // start is the start of the parsed packet
    // cursor is the position for the next byte to read
    // read_until is how far to read in the next step. This is the end of the
    // fixed heard (2 bytes) until msg_incomplete, where it becomes the end of
    // the message.

    // We do not keep a separate marker for the beginning of the mqtt variable header,
    // as it is always start+2.
    // payload start (as beginning of packet payload) belongs into packet parser.

    receiver(cbuf_t read_buffer) : buf{read_buffer},
        start{buf.begin()}, cursor{start}, read_until{start + 2}
    {
    }


    // Packet parsing
    receiver_state state() const;
    void advance();
    void advance(size_t count);
    void advance_until(cbuf_t::iterator);

    // Parse result
    byte msg_type() const;
    cbuf_t content() const;

    void reset();

    // Control over how much still to read
    ptrdiff_t bytes_to_read() const;
    explicit operator bool() const;

private:
    receiver_state m_state = receiver_state::init;
    cbuf_t buf;

    // just a shorthand for buf.begin()
    const cbuf_t::const_iterator start;
    cbuf_t::const_iterator cursor, read_until;

    void consume_byte(byte b);
}; // class receiver

enum class read_result
{
    success,
    more_to_read,
    read_error
};


class Packet_reader{
public:
    struct Receiving_Connection{
        virtual int read(buf_t) = 0;
    };


    Packet_reader(Receiving_Connection& _conn, buf_t _read_buffer) :
        conn{_conn}, read_buffer{_read_buffer}, cursor{read_buffer.begin()},
        rec{read_buffer}
    {}

    read_result read_packet();
    cbuf_t content() const;

private:
    Receiving_Connection& conn;
    buf_t read_buffer;
    buf_t::iterator cursor;
    receiver rec;

};


class Connection
{
public:
    virtual int send(gsl::span<const byte>) = 0;
    // TODO: This does not make sense. If the Connection knows, where the send_buf
    // is, I do not need to pass it to send()
    virtual gsl::span<byte> get_send_buf() = 0;
};

typedef std::function<void(gsl::span<const byte>, gsl::span<const byte>)> callback_t;

// TODO: check, if mikado is actually a sender (mirrored to above receiver class)
// mikado will place the respective packet to send in send_buf, but will not do the sending.
// expecting the buffer to be random accessible, contiguous memory should not be too limiting.
//
// The state has to assume that sending has succeeded after we place it in the send_buf
class mikado_sm
{
public:
    mikado_sm(
            Connection &, callback_t = [](cbuf_t, cbuf_t) {});

    void request_connect(const std::string &clientID);
    void subscribe(const std::string topic);
    void publish(const std::string& topic, const std::string& payload,
                 bool retain = false);
    void publish(cbuf_t topic, cbuf_t payload,
                 bool retain = false);

    void process_packet(cbuf_t packet);
    void send_ping();
    void send_disconnect();

    void reset();

    state_t state() const;

private:
    Connection &conn;
    callback_t cb; // publish callback

    buf_t send_buf;
    state_t m_state = state_t::disconnected;

    // we implement the state machine by having functions for each state we're in
    // they will parse incoming packets and change the state machine state accordingly
    void process_packet_conn_requested(cbuf_t packet_buf);
    void process_packet_subscribe_requested(cbuf_t packet_buf);
    void process_packet_connected(cbuf_t packet_buf);
    void process_packet_ping_await(cbuf_t packet_buf);
    bool handle_publish(cbuf_t packet_buf);
};

}; // namespace mikado

#endif //MIKADO_H_INCLUDED
