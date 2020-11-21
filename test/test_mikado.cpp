#define BOOST_TEST_MODULE mikado test
#include <boost/test/unit_test.hpp>

#include <array>

#include "mikado.h"

using namespace mikado;
namespace m = ::mikado;

BOOST_AUTO_TEST_CASE( mikado_constructor )
{
    int i=5;
    mikado<int> mi{i};

    BOOST_CHECK(mi.state() == m::state_t::disconnected);
}

struct connection_mock
{
    typedef std::vector<unsigned char> buf_t;
    buf_t packet;

    size_t sent_packet_count = 0;

    connection_mock()
    {
    }

    /// Return number of bytes sent, <0 on error
    int send(m::gsl::span<const unsigned char> buf)
    {
        packet.push_back('>');

        std::copy(buf.begin(), buf.end(), std::back_inserter(packet));
        sent_packet_count++;
        return buf.size();
    }

    /// Return number of bytes received, <0 on error
    int recv(m::gsl::span<unsigned char> buf)
    {
        // don't recv() anything
        packet.push_back('<');

        static constexpr m::byte connack_response[] =
        {
            packet_type::connack, 7,
            0, // cannack flags: no session present
            0, // connack reason code: 0 - success
            4 ,// connack property length
            0x24, 0, // propetry: maximum QoS 0
            0x25, 0 // property: no retain available
        };

        static auto cursor = std::begin(connack_response);
        auto rest = m::gsl::make_span(cursor, std::end(connack_response));

        const auto incr = m::copy(cursor, std::end(connack_response),
                                  buf.begin(), buf.end());
        std::copy_n(cursor, incr, std::back_inserter(packet));

        cursor += incr;

        return incr;
    }
};

BOOST_AUTO_TEST_CASE ( mikado_connect )
{
    connection_mock mock;
    auto mi = mikado<connection_mock>{mock};

    mi.request_connect();

    BOOST_CHECK(mi.state() == m::state_t::connection_requested);

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
    BOOST_CHECK_EQUAL_COLLECTIONS(mock.packet.begin(), mock.packet.end(), ref.begin(), ref.end());
}
