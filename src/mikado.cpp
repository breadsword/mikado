#include "mikado.h"

#include "utils.h"
#include "packets.h"

namespace mikado{

receiver_state receiver::state() const
{return m_state;}


void receiver::put_byte(byte b)
{
    switch(m_state){
    case receiver_state::init:
        m_msg_type = b;
        m_state = receiver_state::got_type;
        break;
    case receiver_state::got_type:
        remaining_length = b;
        packet.reserve(remaining_length);

        if (remaining_length == 0){
            m_state = receiver_state::msg_complete;
        }
        else
        {
            m_state = receiver_state::msg_incomplete;
        }
        break;
    case receiver_state::msg_incomplete:
        packet.push_back(b);
        --remaining_length;
        if (remaining_length == 0){
            m_state = receiver_state::msg_complete;
        }
        break;
    default:
        m_state = receiver_state::error;
        break;
    }
}

void receiver::advance()
{
    if (end == std::end(buf))
    {
        // trying to read over end of buffer
        m_state = receiver_state::error;
        return;
    }

    const auto b = *end;
    ++end;

    put_byte(b);

}

void receiver::advance(size_t count)
{
    for(auto r=0; r<count; ++r)
    {
        advance();
    }
}


byte receiver::msg_type() const
{
    return m_msg_type;
}

gsl::span<const byte> receiver::content() const
{
    return gsl::make_span(start, end);
}

}; // namespace mikado
