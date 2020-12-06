#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <fcntl.h>


#include "log.h"
#include "net_util.h"


my_socket::my_socket(int domain, int type, int protocol) : s{socket(domain, type, protocol)}
{
    LOG << "Creating my_socket " << s << " with "
        << " domain  " << domain
        << " type " << type
        << " protocol " << protocol
        << endl;
}

my_socket::my_socket(int sock_fd) :  s{sock_fd}
{
    LOG << "Creating my_socket " << s << endl;
}



my_socket::my_socket(my_socket &&other) : s(-1)
{
    std::swap(s, other.s);
}

my_socket::~my_socket()
{
    if (s>0)
    {
        LOG << "closing socket " << s << endl;
        close(s);
    }
    else
    {
        LOG << "Socket was not open" << endl;
    }
}

void my_socket::set_nonblocking()
{
    const auto flags = fcntl(s,F_GETFL,0);
    if (flags < 0)
    {
        throw(std::runtime_error("Could not get socket flags."));
    }
    const auto r = fcntl(s, F_SETFL, flags | O_NONBLOCK);
    if (r < 0)
    {
        throw(std::runtime_error("Could not set socket non-blocking"));
    }

    LOG << "Set socket non-blocking" << endl;
}

my_socket &my_socket::operator=(my_socket &&other)
{
    std::swap(s, other.s);
    return *this;
}

addr_info::addr_info(const std::string host, const std::string service, addrinfo *hints) : res0(nullptr)
{
    const int error = getaddrinfo(host.c_str(), service.c_str(), hints, &res0);
    if (error) {
        throw(std::runtime_error(gai_strerror(error)));
    }
}

addr_info::~addr_info()
{
    if (res0)
    {
        freeaddrinfo(res0);
    }
}

std::string hostinfo(const sockaddr *sa)
{
    char hbuf[NI_MAXHOST], sbuf[NI_MAXSERV];
    const auto r = getnameinfo(sa, sa->sa_len, hbuf, sizeof(hbuf), sbuf,
                               sizeof(sbuf), NI_NUMERICHOST | NI_NUMERICSERV);
    if (r < 0) {
        throw std::runtime_error{gai_strerror(r)};
    }

    return std::string{hbuf} + "(" + sbuf + ")";
}

my_socket tcp_connect(const std::string host, const  std::string service)
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
