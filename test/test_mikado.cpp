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

    virtual gsl::span<byte> get_send_buf() override
    {
        return gsl::span<byte>();
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
    {}

    /// Return number of bytes sent, <0 on error
    virtual int send(gsl::span<const unsigned char> buf) override
    {
        log.push_back('>');

        std::copy(buf.begin(), buf.end(), std::back_inserter(log));
        sent_packet_count++;
        return buf.size();
    }

};

BOOST_AUTO_TEST_CASE ( mikado_connect_request )
{
    connection_mock mock;
    auto mi = mikado_sm{mock};

    mi.request_connect("client");

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
        0, 6, //string length
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
    mi.request_connect("");
    BOOST_CHECK(mi.state() == state_t::connection_requested);

    mi.process_packet(packet_connack);
    BOOST_CHECK(mi.state() == state_t::connected);
}

BOOST_AUTO_TEST_CASE( mikado_subscribe )
{
    connection_mock mock;
    auto mi = mikado_sm{mock};
    mi.request_connect("");
    mi.process_packet(packet_connack);

    mock.log.clear();
    mi.subscribe("a/b");

    const std::vector<unsigned char> ref = {
        '>',
        packet_type::subscribe | 0x2,
        8, //remaining length
        5, 9, // packet identifier
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

    mi.request_connect("");
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
    auto cb = [&callback_data](cbuf_t t, cbuf_t p){callback_data(t, p);};
    auto mi = mikado_sm{mock,cb};

    mi.request_connect("");
    mi.process_packet(packet_connack);
    BOOST_CHECK(mi.state() == state_t::connected);

    mi.process_packet(packet_publish);
    BOOST_CHECK(mi.state() == state_t::connected);
    BOOST_CHECK(callback_data.called);
    BOOST_CHECK_EQUAL(callback_data.topic, "a/b");
    BOOST_CHECK_EQUAL(callback_data.payload, "Hello");
}

BOOST_AUTO_TEST_CASE( mikado_receive_publish_no_payload )
{
    const std::vector<byte> packet_publish
    {
        packet_type::publish | 0, // DUP false, QoS 0, Retain false
        5,
        0, 3, 'a', '/', 'b' // topic
                // no payload
    };

    connection_mock mock;
    callback_mock callback_data;
    auto cb = [&callback_data](cbuf_t t, cbuf_t p){callback_data(t, p);};
    auto mi = mikado_sm{mock,cb};

    mi.request_connect("");
    mi.process_packet(packet_connack);
    BOOST_CHECK(mi.state() == state_t::connected);

    mi.process_packet(packet_publish);
    BOOST_CHECK(mi.state() == state_t::connected);
    BOOST_CHECK(callback_data.called);
    BOOST_CHECK_EQUAL(callback_data.topic, "a/b");
    BOOST_CHECK_EQUAL(callback_data.payload, "");

}

BOOST_AUTO_TEST_CASE( mikado_set_callback )
{
    connection_mock mock;
    // create without callback
    auto mi = mikado_sm{mock};

    mi.request_connect("");
    mi.process_packet(packet_connack);
    BOOST_CHECK(mi.state() == state_t::connected);

    mi.process_packet(packet_publish);
    BOOST_CHECK(mi.state() == state_t::connected);

    callback_mock callback_data;
    auto cb = [&callback_data](cbuf_t t, cbuf_t p){callback_data(t, p);};
    BOOST_CHECK(!callback_data.called);

    mi.set_callback(cb);
    mi.process_packet(packet_publish);
    BOOST_CHECK(mi.state() == state_t::connected);
    BOOST_CHECK(callback_data.called);
    BOOST_CHECK_EQUAL(callback_data.topic, "a/b");
    BOOST_CHECK_EQUAL(callback_data.payload, "Hello");
}

BOOST_AUTO_TEST_CASE( mikado_callback_calls_publish )
{
    connection_mock mock;
    auto mi = mikado_sm{mock};
    bool called = false;
    auto cb = [&mi, &called](cbuf_t t, cbuf_t v){mi.publish(t, v); called = true;};
    mi.set_callback(cb);

    mi.request_connect("");
    mi.process_packet(packet_connack);
    BOOST_CHECK(mi.state() == state_t::connected);

    mock.sent_packet_count = 0;
    mock.log.clear();
    mi.process_packet(packet_publish);
    // now, the callback is called, which in turn sends back the packet
    // mikado_sm should not be confused by this
    BOOST_CHECK(mi.state() == state_t::connected);

    BOOST_CHECK(called);
    BOOST_CHECK_EQUAL(mock.sent_packet_count, 1);
    // The echo'd packet is in mock.log, after the initial '>'
    BOOST_CHECK_EQUAL_COLLECTIONS(packet_publish.begin(), packet_publish.end(),
                                  mock.log.begin()+1, mock.log.end());
}

BOOST_AUTO_TEST_CASE( mikado_send_publish )
{
    connection_mock mock;
    auto mi = mikado_sm{mock};

    mi.request_connect("");
    mi.process_packet(packet_connack);
    BOOST_CHECK(mi.state() == state_t::connected);

    mock.log.clear();
    // const std::vector<byte> t = {'a','/','b'};
//    const unsigned char t[]{"a/b"};
//    const std::vector<byte> p = {'t', 'h', 'i', 's'};
    mi.publish("a/b", "this");
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

BOOST_AUTO_TEST_CASE( mikado_send_ping )
{
    connection_mock mock;
    auto mi = mikado_sm{mock};
    mi.request_connect("");
    mi.process_packet(packet_connack);
    BOOST_CHECK(mi.state() == state_t::connected);

    mock.log.clear();

    mi.send_ping();

    const std::vector<byte> ref =
    {
        '>',
        packet_type::pingreq,
        0
    };
    BOOST_CHECK(mi.state() == state_t::ping_await );
    BOOST_CHECK_EQUAL_COLLECTIONS(mock.log.begin(), mock.log.end(),
                                  ref.begin(), ref.end());
}

const std::vector<byte> packet_pingresp
{
    packet_type::pingresp,
            0
};

BOOST_AUTO_TEST_CASE( mikado_receive_pingresp )
{
    connection_mock mock;
    auto mi = mikado_sm{mock};
    mi.request_connect("");
    mi.process_packet(packet_connack);
    mi.send_ping();
    BOOST_CHECK(mi.state() == state_t::ping_await );

    mi.process_packet(packet_publish);
    BOOST_CHECK(mi.state() == state_t::ping_await );

    mi.process_packet(packet_pingresp);
    BOOST_CHECK(mi.state() == state_t::connected);
}

BOOST_AUTO_TEST_CASE( mikado_send_disconnect )
{
    connection_mock mock;
    auto mi = mikado_sm{mock};
    mi.request_connect("");
    mi.process_packet(packet_connack);
    BOOST_CHECK(mi.state() == state_t::connected);

    const std::vector<byte> ref =
    {
        '>',
        packet_type::disconnect,
        0
    };

    mock.log.clear();
    mi.send_disconnect();
    BOOST_CHECK(mi.state() == state_t::disconnected);
    BOOST_CHECK_EQUAL_COLLECTIONS(mock.log.begin(), mock.log.end(),
                                  ref.begin(), ref.end());
}


BOOST_AUTO_TEST_CASE( incremental_lexing )
{

    // byte 2 is remaining length
    const byte input[] = {1, 3, 2, 4, 5};
    const byte *it = &input[0];
    receiver rec{input};

    while(rec)
    {
        it  += rec.bytes_to_read();
        rec.advance_until(it);
    }
    BOOST_CHECK(rec.state() == receiver_state::msg_complete);

}

BOOST_AUTO_TEST_CASE( receiver_len1 )
{
    const byte msg[] = {42, 1, 4};
    receiver r(msg);

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


struct Receiving_connection_mock : public Packet_reader::Receiving_Connection
{
    virtual int read(buf_t b) override
    {
        switch (pass)
        {
        case 0:
            // place stuff in b
            ++pass;
            return copy(p.begin(), p.begin()+2, b.begin(), b.end());

            break;
        case 1:
            // return content
            ++pass;
            return copy (p.begin()+2, p.end(), b.begin(), b.end());
            break;
        case 2:
            return 0;
        case 3:
            return -1;
        }

        return -1;
    }

    const std::vector<byte> p =
    {
        0, 3,
        1, 2, 5
    };

    int pass = 0;

};

BOOST_AUTO_TEST_CASE( receiver_partial_packet )
{
    Receiving_connection_mock mock;
    std::array<byte, 258> buf;
    Packet_reader r{mock, buf};

    auto ret = r.read_packet();
    BOOST_CHECK(ret == read_result::more_to_read);
    ret = r.read_packet();
    BOOST_CHECK(ret == read_result::success);

    const auto p = r.content();
    BOOST_CHECK_EQUAL_COLLECTIONS(
                mock.p.begin(), mock.p.end(),
                p.begin(), p.end());
}

BOOST_AUTO_TEST_CASE( receiver_packet_loop )
{
    Receiving_connection_mock mock;
    std::array<byte, 258> buf;
    Packet_reader r{mock, buf};

    read_result ret;
    do{
        ret = r.read_packet();
    }while(ret == read_result::more_to_read);
    BOOST_CHECK(ret == read_result::success);
}

struct Receiving_connection_mock2 : public Packet_reader::Receiving_Connection
{
    virtual int read(buf_t b) override
    {
        if (b.size() == 0)
        {
            BOOST_FAIL("read called with 0 bytes buffer.");
            return -1;
        }

        const auto incr = copy(cursor, data.end(), b.begin(), b.end());
        cursor += incr;
        return incr;
    }

    std::vector<byte> data =
    {
        0, 2, 1, 1,
        1, 3, 2, 2, 2
    };
    decltype(data)::iterator cursor = data.begin();
};

BOOST_AUTO_TEST_CASE( receive_multiple_packets )
{
    Receiving_connection_mock2 mock;

    std::array<byte, 1024> read_buf{0x42};


    Packet_reader reader{mock, read_buf};
    for (int i=0; i<2; ++i)
    {
        read_result ret;

        do{
            ret = reader.read_packet();
        }
        while (ret == read_result::more_to_read);
        BOOST_CHECK(ret == read_result::success);
        const auto p = reader.content();

        reader.reset();
    }

}
