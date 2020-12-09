#include "mikado.h"
#include "mikado_util.h"

#include <boost/io/ios_state.hpp>

#include "net_util.h"
#include <sys/socket.h>
#include <netdb.h>
#include "log.h"

namespace m = mikado;

std::ostream& operator<< (std::ostream& os, m::cbuf_t s)
{
    boost::io::ios_all_saver ifs(os);
    for (const auto c : s)
    {
        os << std::hex << std::showbase <<  (int)c << " '" << std::dec << c << "' ";
    }
    return os;
}

void logging_cb(m::cbuf_t t, m::cbuf_t v)
{
    LOG << "Callback called. "
        << "Topic: " << t
        << "Value: " << v
        << endl;
}

int main(int argc, char **argv)
{
    // create socket to connect to mqtt broker
    // construct a connection object
    Socket_connection conn{tcp_connect("localhost", "1883")};

    // construct a mikado
    auto mi = mikado::mikado_sm{conn, logging_cb};

    // connect the mikado
    mi.request_connect("test_client");

    mi.process_packet(recv_packet(conn));

    LOG << "Trying to subscribe" << endl;
    mi.subscribe("/testtopic/a");
    LOG << "Subscribe requested" << endl;
    const auto sub_response = recv_packet(conn);

    LOG << "sub_response: " << sub_response << endl;

    mi.process_packet(sub_response);
    LOG << "mikado state: " <<  (int)mi.state() << endl;

    if (mi.state() == m::state_t::connected)
    {
        LOG << "Subscribe successful" << endl;
    }

    mi.publish("/testpublish", "value");
    LOG << "published message" << endl;
    if (mi.state() == m::state_t::connected)
    {
        LOG << "Published. Mikado_sm remains connected." << endl;
    }

    LOG << "waiting for publish" << endl;
    mi.process_packet(recv_packet(conn));
    if (mi.state() == m::state_t::connected)
    {
        LOG << "Received published message" << endl;
    }

    mi.send_ping();
    mi.process_packet(recv_packet(conn));
    if (mi.state() == m::state_t::connected)
    {
        LOG << "Received ping response" << endl;
    }

    mi.send_disconnect();

    return 0;
}
