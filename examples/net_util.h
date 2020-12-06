#ifndef NET_UTIL_H_INCLUDED
#define NET_UTIL_H_INCLUDED

#include <string>

/// Handle to a C socket which can be returned from a function
///
/// Uses RAII to ensure socket is closed.
struct my_socket
{
    my_socket(int domain, int type, int protocol);
    my_socket(int sock_fd);

    void set_nonblocking();

    my_socket(my_socket&) = delete;
    void operator= (my_socket&) = delete;
    my_socket& operator=(my_socket &&);
    my_socket(my_socket&&);
    ~my_socket();

    int s;
};

struct addrinfo;

/// RAII generation of struct addrinfo list from getaddrinfo() automatic deallocation
struct addr_info
{
    addr_info(const std::string host, const  std::string service,
              struct addrinfo *hints);
    ~addr_info();

    struct addrinfo *res0;
};

struct sockaddr;
std::string hostinfo(const sockaddr* sa);

my_socket tcp_connect(const std::string host, const  std::string service);


#endif //NET_UTIL_H_INCLUDED
