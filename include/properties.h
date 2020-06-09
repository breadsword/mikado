#ifndef MIKADO_PROPERTIES_H_INCLUDED
#define MIKADO_PROPERTIES_H_INCLUDED

#include <memory>

#include <utils.h>

namespace mikado {
typedef const gsl::span<const byte>& span_t;

class Property{
public:
    virtual ~Property() {}

};

namespace properties {
template <byte id> struct Property_id : public Property
{
    static constexpr byte identifier = id;
};

template <typename value_type, byte id> class gen_prop : public Property_id<id>
{
public:
    static std::unique_ptr<Property> from_span(span_t);
    value_type value;
};

// partial specialization
template <byte id> class gen_prop<byte, id> : public Property_id<id>
{
public:
    static std::unique_ptr<Property> from_span(span_t d)
    {
        if (d.size() != 2)
        {
            return nullptr;
        }

        auto res = std::make_unique<gen_prop<byte, id>>();
        res->value = d[1];
        return res;
    }

    byte value;
};

template <byte id> class gen_prop<uint32_t, id> : public Property_id<id>
{
public:
    static std::unique_ptr<Property> from_span(span_t d)
    {
        if (d.size() != 5)
        {
            return nullptr;
        }

        auto res = std::make_unique<gen_prop<uint32_t, id>>();
        res->value = (d[1] << 24) + (d[2] << 16) + (d[3] << 8) + d[4];
        return res;
    }

    uint32_t value;
};

template <byte id> class gen_prop<std::string, id> : public Property_id<id>
{
public:
    static std::unique_ptr<Property> from_span(span_t d)
    {
        const auto len = (d[1] << 8) + d[2];

        if (d.size() != 1 + 2 + len)
        {
            // wrong length
            return nullptr;
        }

        auto res = std::make_unique<gen_prop<std::string, id>>();
        res->value.reserve(len);
        const auto str_span = d.subspan(3);
        std::copy(str_span.begin(), str_span.end(), std::back_inserter(res->value));

        return res;
    }

    std::string value;
};


// Packet specifications
typedef gen_prop<byte, 0x01> Payload_Format_Indicator;
typedef gen_prop<uint32_t, 0x02> Message_Expiry_Interval;
typedef gen_prop<std::string, 0x03> Content_Type;
typedef gen_prop<byte, 0x24> Maximum_QoS;

} // namespace properties

std::unique_ptr<Property> get_prop(span_t);

}

#endif //MIKADO_PROPERTIES_H_INCLUDED
