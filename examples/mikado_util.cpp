#include "mikado_util.h"

#include <sys/socket.h>

#include "log.h"

namespace m = mikado;

Socket_connection::Socket_connection(my_socket &&_sock) : sock{std::move(_sock)}
{}

int Socket_connection::send(mikado::cbuf_t msg)
{
    LOG << "Sending " << msg.size_bytes() << " bytes." << endl;
    return ::send(sock.s, msg.data(), msg.size_bytes(), 0);
}

int Socket_connection::read(mikado::buf_t b)
{
    auto p = b.data();
    auto len = b.size_bytes();
    const auto r = ::recv(sock.s, p, len, 0);
    if (r > 0)
    {
        return r;
    }
    if (r < 0 && errno == EAGAIN)
    {
        return 0;
    }
    if (r == 0)
    {
        LOG << "Peer closed connection." << endl;
        return -1;
    }
    LOG << "during read: " << strerror(errno) << endl;
    return r;
}

mikado::buf_t Socket_connection::get_send_buf()
{
    return send_buffer;
}

mikado::cbuf_t recv_packet(Socket_connection &conn)
{
    m::Packet_reader reader{conn, conn.recv_buf};
    m::read_result ret;
    do{
        ret = reader.read_packet();
    }while(ret == m::read_result::more_to_read);

    LOG << "read_packet() returned: " << static_cast<int>(ret) << endl;

    return reader.content();
}
