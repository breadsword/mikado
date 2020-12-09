#include <thread>
#include <chrono>

#include "mikado_util.h"
#include "log.h"

namespace m = mikado;

void logging_cb(m::cbuf_t t, m::cbuf_t v)
{
    std::string s_t, s_v;
    std::copy(t.begin(), t.end(), std::back_inserter(s_t));
    std::copy(v.begin(), v.end(), std::back_inserter(s_v));

    LOG << "Topic: " << s_t << " value: " << s_v << endl;
}

struct execution_context{
    // create socket to connect to mqtt broker
    // construct a connection object
    Socket_connection conn{tcp_connect("localhost", "1883")};
    m::mikado_sm mi{conn, logging_cb};


    void setup()
    {
        LOG << "Setup" << endl;

        conn.sock.set_nonblocking();

        mi.request_connect("logging_client");
        const auto p = recv_packet(conn);
        mi.process_packet(p);
        if (mi.state() != m::state_t::connected)
        {
            throw std::runtime_error("Could not connect mikado");
        }

        mi.subscribe("#");
        mi.process_packet(recv_packet(conn));
        if (mi.state() != m::state_t::connected)
        {
            throw std::runtime_error("Could not subscribe mikado");
        }
    }


    m::Packet_reader reader{conn, conn.recv_buf};

    static constexpr std::chrono::duration<double> timeout{5.0};

    void loop()
    {
        static auto last_read = std::chrono::system_clock::now();

        const auto ret = reader.read_packet();
        const auto now = std::chrono::system_clock::now();

        if (ret == m::read_result::success)
        {
            mi.process_packet(reader.content());
            last_read = now;
        }
        if (ret == m::read_result::read_error)
        {
            throw std::runtime_error("Read error");
        }


        if (now - last_read > timeout)
        {
            LOG << "No packet for " << timeout.count() << " seconds." << endl;
            last_read = now;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }
}; // Execution_context

constexpr std::chrono::duration<double> execution_context::timeout;

int main(int argc, char *argv[])
{
    execution_context context;

    context.setup();

    while(true)
    {
        context.loop();
    }

    return 0;
}
