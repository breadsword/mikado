#ifndef MIKADO_H_INCLUDED
#define MIKADO_H_INCLUDED

#include <gsl-lite/gsl-lite.hpp>

#include <packets.h>
#include <utils.h>
#include <vbi.h>

namespace mikado {

// namespace gsl = ::gsl_lite; // convenience alias

enum class state_t{
    disconnected,
    connection_requested,
    connected,
    subscribe_requested,
    ping_await,
    error
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

}; // class receiver

class Connection{
public:
    virtual int send(gsl::span<const byte>) = 0;
    virtual gsl::span<byte> get_send_buf() = 0;
    // virtual gsl::span<byte> recv() = 0;
};

typedef std::function<void(gsl::span<const byte>, gsl::span<const byte>)> callback_t;

// TODO: check, if mikado is actually a sender (mirrored to above receiver class)
// mikado will place the respective packet to send in send_buf, but will not do the sending.
// expecting the buffer to be random accessible, contiguous memory should not be too limiting.
//
// The state has to assume that sending has succeeded after we place it in the send_buf
class mikado_sm{
public:
    mikado_sm(Connection&, callback_t=[](auto, auto){});

    void request_connect();
    void subscribe(const std::string topic);
    void publish(gsl::span<const byte> topic, gsl::span<const byte> payload,
                 bool retain=false);

    void process_packet(gsl::span<const byte> packet);
    void send_ping();

    state_t state() const;

private:
    Connection& conn;
    callback_t cb; // publish callback

    gsl::span<byte> send_buf;
    state_t m_state = state_t::disconnected;

    // we implement the state machine by having functions for each state we're in
    // they will parse incoming packets and change the state machine state accordingly
    void process_packet_conn_requested(gsl::span<const byte> packet_buf);
    void process_packet_subscribe_requested(gsl::span<const byte> packet_buf);
    void process_packet_connected(gsl::span<const byte> packet_buf);
};


}; // namespace mikado

#endif //MIKADO_H_INCLUDED
