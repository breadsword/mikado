#ifndef NET_UTIL_H_INCLUDED
#define NET_UTIL_H_INCLUDED

#include <string>

/// Handle to a C socket which can be returned from a function
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
struct addr_info
{
    addr_info(const char *host, const char *service, struct addrinfo *hints);

    ~addr_info();

    struct addrinfo *res0;
};

struct sockaddr;
std::string hostinfo(const sockaddr* sa);

#endif //NET_UTIL_H_INCLUDED
