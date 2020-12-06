#include "mikado.h"

#include <boost/io/ios_state.hpp>

#include "net_util.h"
#include <sys/socket.h>
#include <netdb.h>
#include "log.h"

namespace m = mikado;


struct Socket_connection : public m::Connection, m::Packet_reader::Receiving_Connection
{
    Socket_connection(my_socket&& _sock) : sock{std::move(_sock)}
    {}


    virtual int send(m::cbuf_t msg) override
    {
        LOG << "Sending " << msg.size_bytes() << " bytes." << endl;
        return ::send(sock.s, msg.data(), msg.size_bytes(), 0);
    }

    virtual int read(m::buf_t b) override
    {
        return ::recv(sock.s, b.data(), b.size_bytes(), 0);
    }

    my_socket sock;

    virtual m::buf_t get_send_buf() override
    {
        return send_buffer;
    }
    std::array<m::byte, 258> send_buffer{0};
};

std::array<m::byte, 258> recv_buf{0};
m::cbuf_t recv_packet(Socket_connection& conn)
{
    m::Packet_reader reader{conn, recv_buf};
    m::read_result ret;
    do{
        ret = reader.read_packet();
    }while(ret == m::read_result::more_to_read);
    return reader.content();
}

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
