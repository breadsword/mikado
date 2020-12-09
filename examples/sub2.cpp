#include <thread>
#include <chrono>

#include "mikado.h"
#include "net_util.h"
#include "mikado_util.h"
#include "log.h"

namespace m = mikado;


void logging_cb(m::cbuf_t _t, m::cbuf_t _v)
{
    std::string t, v;

    std::copy(_t.begin(), _t.end(), std::back_inserter(t));
    std::copy(_v.begin(), _v.end(), std::back_inserter(v));

    LOG << "Callback called. "
        << "Topic: " << t
        << " Value: " << v
        << endl;
}


int main(int argc, char**argv)
{
    Socket_connection conn{tcp_connect("localhost", "1883")};
    conn.sock.set_nonblocking();

    // construct a mikado
    auto mi = m::mikado_sm{conn, logging_cb};

    // connect the mikado
    mi.request_connect("test_client");
    mi.process_packet(recv_packet(conn));
    assert(mi.state() == m::state_t::connected);
    LOG << "Connected" << endl;

    mi.subscribe("/testtopic/#");
    const auto sub_response = recv_packet(conn);
    mi.process_packet(sub_response);
    assert(mi.state() == m::state_t::connected);
    LOG << "Subscribe successful" << endl;

    constexpr std::chrono::duration<double> timeout{5};
    auto last_read = std::chrono::system_clock::now();

    while(true)
    {

        m::Packet_reader reader{conn, conn.recv_buf};
        m::read_result ret;
        auto now = std::chrono::system_clock::now();
        do
        {
            now = std::chrono::system_clock::now();
            if ((now - last_read) > timeout)
            {
                LOG << "No packet for " << timeout.count() << " seconds." << endl;
                last_read = now;
            }

            ret = reader.read_packet();
            std::this_thread::sleep_for(std::chrono::milliseconds(200));
        }while (ret == m::read_result::more_to_read);

        if (ret == m::read_result::success)
        {
            const auto p = reader.content();
            LOG << "Packet complete. Packet is " << p.size() << " bytes." << endl;
            mi.process_packet(p);
            last_read = now;
        }

        if (ret == m::read_result::read_error)
        {
            LOG << "Caught read error. " << strerror(errno) << endl;
            break;
        }
    }
    LOG << "Exited packet loop" << endl;

    LOG << "disconnecting" << endl;
    mi.send_disconnect();

    return 0;
}
