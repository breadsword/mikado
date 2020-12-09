#ifndef MIKADO_UTIL_H
#define MIKADO_UTIL_H

#include "mikado.h"
#include "net_util.h"

struct Socket_connection : public mikado::Connection, mikado::Packet_reader::Receiving_Connection
{
    Socket_connection(my_socket&& _sock);

    virtual int send(mikado::cbuf_t msg) override;

    virtual int read(mikado::buf_t b) override;

    my_socket sock;

    virtual mikado::buf_t get_send_buf() override;
    std::array<mikado::byte, 258> send_buffer{0};
    std::array<mikado::byte, 258> recv_buf{0};
};

mikado::cbuf_t recv_packet(Socket_connection& conn);

#endif //MIKADO_UTIL_H
