#define BOOST_TEST_MODULE mikado test
#include <boost/test/unit_test.hpp>

#include <array>

#include <gsl-lite/gsl-lite.hpp>

#include "mikado.h"

using namespace mikado;
namespace m = ::mikado;

BOOST_AUTO_TEST_CASE( mikado_constructor )
{
    int i=5;
    mikado<int> mi{i};

    BOOST_CHECK(!mi.connected());
}

struct connection_mock
{
    // TODO: I need to pass a send_buffer out, which is copied into my log
    // on flush. In this way, I know which direction the exchange is meant to
    // pass.
    // TODO: How do I figure out, how much data was written into the buffer?
    // -> Through send() or flush() calls.

    typedef std::vector<unsigned char> buf_t;
    buf_t packet;

    size_t sent_packet_count = 0;

    connection_mock()
    {
    }

    /// Return number of bytes sent
    int send(m::gsl::span<const unsigned char> buf)
    {
        packet.push_back('>');

        std::copy(buf.begin(), buf.end(), std::back_inserter(packet));
        sent_packet_count++;
        return buf.size();
    }

    /// Return number of bytes received, < 0 on error
    int recv(m::gsl::span<unsigned char> buf)
    {
        // don't recv() anything
        packet.push_back('<');
        return -1;
    }


};

BOOST_AUTO_TEST_CASE ( mikado_connect )
{
    connection_mock mock;
    auto mi = mikado<connection_mock>{mock};

    mi.connect();

    // 2020-06-04: with current recv() mock, no connection will work
    BOOST_CHECK(!mi.connected());

    // ensure, that mikado sends something out when it is meant to connect
    BOOST_CHECK_GT(mock.sent_packet_count, 0);

    // check content of connect packet (??)
    const std::vector<unsigned char> ref = {
        '>',
        packet_type::connect, 18,
        0, 4, // string length
        'M', 'Q', 'T', 'T',  // string
        5, // protocol version
        2, // clean_start
        0, 0, // keep alive
        0, // no properties
        6, //string length
        'c', 'l', 'i', 'e', 'n', 't',
        '<' // try to get response
    };
    BOOST_CHECK_EQUAL_COLLECTIONS(mock.packet.begin(), mock.packet.end(), ref.begin(), ref.end());
}
