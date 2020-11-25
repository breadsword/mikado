#include "mikado.h"

#include <boost/io/ios_state.hpp>

#include "net_util.h"
#include <sys/socket.h>
#include <netdb.h>
#include "log.h"

namespace m = mikado;

my_socket my_connect(const char* host, const char*service)
{
    // resolve host:service
    struct addrinfo hints;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = PF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    addr_info infos(host, service, &hints);
    // try to connect to each in turn
    for (auto res = infos.res0; res; res = res->ai_next) {

        auto sock = my_socket{res->ai_family, res->ai_socktype,
                res->ai_protocol};
        if (sock.s < 0) {
            continue;
        }

        const auto r = connect(sock.s, res->ai_addr, res->ai_addrlen);
        if ( r < 0) {
            LOG << "connect failed: " << strerror(errno) << endl;
            continue;
        }

        return sock;
    }
    throw(std::runtime_error("Could not connect with any address."));
    return my_socket(-1);
}

struct Socket_connection : public m::Connection
{
    Socket_connection(my_socket&& _sock) : sock{std::move(_sock)}
    {}


    virtual gsl::span<m::byte> get_send_buf() override
    {
        return send_buffer;
    }

    virtual int send(gsl::span<const m::byte> msg) override
    {
        LOG << "Sending " << msg.size_bytes() << " bytes." << endl;
        return ::send(sock.s, msg.data(), msg.size_bytes(), 0);
    }

    my_socket sock;
    std::array<m::byte, 258> send_buffer{0};
};



std::array<m::byte, 258> recv_buf{0};

gsl::span<const m::byte> recv_packet(Socket_connection& conn)
{
    // Receive header
    const auto r = recv(conn.sock.s, &recv_buf[0], 2, 0);
    LOG << "recv r: " << r << endl;
    m::receiver rec{recv_buf};
    rec.advance(2);

    // receive rest of packet
    while (rec.state() == m::receiver_state::msg_incomplete)
    {
        const auto r2 = recv(conn.sock.s, &recv_buf[2], recv_buf.size()-2, 0);
        LOG << "recv r2: " << r2 << endl;
        rec.advance(r2);
    }

    if (rec.state() == m::receiver_state::msg_complete)
    {
        LOG << "received complete message" << endl;
    }
    return rec.content();
}

std::ostream& operator<< (std::ostream& os, gsl::span<const m::byte> s)
{
    boost::io::ios_all_saver ifs(os);
    for (const auto c : s)
    {
        os << std::hex << std::showbase <<  (int)c << " '" << std::dec << c << "' ";
    }
    return os;
}

void logging_cb(gsl::span<const m::byte> t, gsl::span<const m::byte> v)
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
    Socket_connection conn{my_connect("localhost", "1883")};

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

    const unsigned char topic[] = "/testpublish";
    auto t_span = gsl::span<const m::byte>(topic, sizeof(topic)-1);
    const unsigned char value[] = "value";
    auto v_span = gsl::span<const m::byte>(value, sizeof(value)-1);
    mi.publish(t_span, v_span);
    LOG << "published message" << endl;
    if (mi.state() == m::state_t::connected)
    {
        LOG << "Published. Mikado_sm is connected." << endl;
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
