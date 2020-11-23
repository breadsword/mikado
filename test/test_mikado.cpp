#define BOOST_TEST_MODULE mikado test
#include <boost/test/unit_test.hpp>

#include <array>

#include "mikado.h"

using namespace mikado;

struct simple_connection_mock : public Connection
{
    virtual int send(gsl::span<const byte> msg) override
    {
        return -1;
    }

    std::array<byte, 256> send_buf;
    virtual gsl::span<byte> get_send_buf() override
    {
        return send_buf;
    }
};

BOOST_AUTO_TEST_CASE( mikado_constructor )
{
    simple_connection_mock v;
    mikado_sm mi{v};

    BOOST_CHECK(mi.state() == state_t::disconnected);
}

struct connection_mock : public Connection
{

    typedef std::vector<unsigned char> log_t;
    log_t log;

    std::array<byte, 1024> send_buffer;
    virtual gsl::span<byte> get_send_buf() override
    {
        return send_buffer;
    }

    size_t sent_packet_count = 0;

    connection_mock()
    {
    }

    /// Return number of bytes sent, <0 on error
    virtual int send(gsl::span<const unsigned char> buf) override
    {
        log.push_back('>');

        std::copy(buf.begin(), buf.end(), std::back_inserter(log));
        sent_packet_count++;
        return buf.size();
    }

    /// Return number of bytes received, <0 on error
    int recv(gsl::span<unsigned char> buf)
    {
        // don't recv() anything
        log.push_back('<');

        static constexpr byte connack_response[] =
        {
            packet_type::connack, 7,
            0, // cannack flags: no session present
            0, // connack reason code: 0 - success
            4 ,// connack property length
            0x24, 0, // propetry: maximum QoS 0
            0x25, 0 // property: no retain available
        };

        const auto incr = copy(std::begin(connack_response), std::end(connack_response),
                                  buf.begin(), buf.end());
        std::copy_n(std::begin(connack_response), incr, std::back_inserter(log));

        return incr;
    }
};

BOOST_AUTO_TEST_CASE ( mikado_connect_request )
{
    connection_mock mock;
    auto mi = mikado_sm{mock};

    mi.request_connect();

    BOOST_CHECK(mi.state() == state_t::connection_requested);

    // ensure, that mikado sends exactly one packet when it connects
    BOOST_CHECK_EQUAL(mock.sent_packet_count, 1);

    // check content of packet exchange
    const std::vector<unsigned char> ref = {
        '>',
        packet_type::connect, 18,
        0, 4, // string length
        'M', 'Q', 'T', 'T',  // string
        4, // protocol version 3.1.1 (0x04)
        1 << 1, // clean Session true
        0, 0, // keep alive
        0, // no properties
        6, //string length
        'c', 'l', 'i', 'e', 'n', 't',
    };
    BOOST_CHECK_EQUAL_COLLECTIONS(mock.log.begin(), mock.log.end(), ref.begin(), ref.end());
}

const std::vector<byte> packet_connack =
{
    packet_type::connack, 2,
    0, 0
};

BOOST_AUTO_TEST_CASE ( mikado_connect )
{
    connection_mock mock;
    auto mi = mikado_sm{mock};
    mi.request_connect();
    BOOST_CHECK(mi.state() == state_t::connection_requested);

    mi.process_packet(packet_connack);
    BOOST_CHECK(mi.state() == state_t::connected);
}

BOOST_AUTO_TEST_CASE( mikado_subscribe )
{
    connection_mock mock;
    auto mi = mikado_sm{mock};
    mi.request_connect();
    mi.process_packet(packet_connack);

    mock.log.clear();
    mi.subscribe("a/b");

    const std::vector<unsigned char> ref = {
       '>',
        packet_type::subscribe | 0x2,
        8, //remaining length
        0, 1, // packet identifier
        0, 3, // length of topic
        'a', '/', 'b', // topic_filter
        0 // QoS
    };

    BOOST_CHECK_EQUAL_COLLECTIONS(mock.log.begin(), mock.log.end(), ref.begin(), ref.end());
    BOOST_CHECK(mi.state() == state_t::subscribe_requested);
}

const std::vector<byte> packet_suback =
{
    packet_type::suback,
    3, // remaining_length
    0, 1, // packet_identifier
    0 // success, maximum QoS 0
};

BOOST_AUTO_TEST_CASE( mikado_subscribe_confirm )
{
    connection_mock mock;
    auto mi = mikado_sm{mock};

    mi.request_connect();
    mi.process_packet(packet_connack);
    mi.subscribe("a/b");
    BOOST_CHECK(mi.state() == state_t::subscribe_requested);
    mi.process_packet(packet_suback);
    BOOST_CHECK(mi.state() == state_t::connected);
}

const std::vector<byte> packet_publish =
{
    packet_type::publish | 0, // DUP false, QoS 0, Retain false
    10,
    0, 3, 'a', '/', 'b', // topic
    'H', 'e', 'l', 'l', 'o'  // payload, no length encoded
};

struct callback_mock{
    bool called = false;
    std::string topic{""};
    std::string payload{""};

    void operator()(gsl::span<const byte> t, gsl::span<const byte> p)
    {
        called = true;
        std::copy(std::begin(t), std::end(t), std::back_inserter(topic));
        std::copy(std::begin(p), std::end(p), std::back_inserter(payload));
    }
};



BOOST_AUTO_TEST_CASE( mikado_receive_publish )
{

    connection_mock mock;
    callback_mock callback_data;
    auto mi = mikado_sm{mock, [&callback_data](auto t, auto p){callback_data(t, p);}};

    mi.request_connect();
    mi.process_packet(packet_connack);
    BOOST_CHECK(mi.state() == state_t::connected);

    mi.process_packet(packet_publish);
    BOOST_CHECK(mi.state() == state_t::connected);
    BOOST_CHECK(callback_data.called);
    BOOST_CHECK_EQUAL(callback_data.topic, "a/b");
    BOOST_CHECK_EQUAL(callback_data.payload, "Hello");
}

BOOST_AUTO_TEST_CASE( mikado_send_publish )
{
    connection_mock mock;
    auto mi = mikado_sm{mock};

    mi.request_connect();
    mi.process_packet(packet_connack);
    BOOST_CHECK(mi.state() == state_t::connected);

    mock.log.clear();
    const std::vector<byte> t = {'a','/','b'};
    const std::vector<byte> p = {'t', 'h', 'i', 's'};
    mi.publish(t, p);
    BOOST_CHECK(mi.state() == state_t::connected);

    const std::vector<byte> ref =
    {
        '>',
        packet_type::publish,
        9, //remaining length
        0, 3, 'a', '/', 'b', //topic
        't', 'h', 'i', 's' // payload
    };

    BOOST_CHECK_EQUAL_COLLECTIONS(mock.log.begin(), mock.log.end(),
                                  ref.begin(), ref.end());
}

BOOST_AUTO_TEST_CASE( receiver_len1 )
{
    const byte msg[] = {42, 1, 4};
    receiver r(gsl::make_span(msg));

    r.advance();
    BOOST_CHECK(r.state() == receiver_state::got_type);
    r.advance();
    BOOST_CHECK(r.state() == receiver_state::msg_incomplete);
    r.advance();
    BOOST_CHECK(r.state() == receiver_state::msg_complete);

    BOOST_CHECK_EQUAL(r.msg_type(), 42);
    BOOST_CHECK_EQUAL(r.content()[2], 4);

}

BOOST_AUTO_TEST_CASE( receiver_len0 )
{
    const byte msg[] = {42, 0};
    receiver r(msg);
    r.advance(2);
    BOOST_CHECK(r.state() == receiver_state::msg_complete);
}

