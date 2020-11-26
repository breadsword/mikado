#if 0

#ifndef MIKADO_PROPERTIES_H_INCLUDED
#define MIKADO_PROPERTIES_H_INCLUDED

#include <memory>

#include <vbi.h>
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
        if (d.size() != 1)
        {
            return nullptr;
        }

        auto res = std::make_unique<gen_prop<byte, id>>();
        res->value = d[0];
        return res;
    }

    byte value;
};

template <byte id> class gen_prop<uint32_t, id> : public Property_id<id>
{
public:
    static std::unique_ptr<Property> from_span(span_t d)
    {
        if (d.size() != 4)
        {
            return nullptr;
        }

        auto res = std::make_unique<gen_prop<uint32_t, id>>();
        res->value = (d[0] << 24) + (d[1] << 16) + (d[2] << 8) + d[3];
        return res;
    }

    uint32_t value;
};

template <byte id> class gen_prop<std::string, id> : public Property_id<id>
{
public:
    static std::unique_ptr<Property> from_span(span_t d)
    {
        const auto len = (d[0] << 8) + d[1];

        if (d.size() != 2 + len)
        {
            // wrong length
            return nullptr;
        }

        auto res = std::make_unique<gen_prop<std::string, id>>();
        res->value.reserve(len);
        const auto str_span = d.subspan(2);
        std::copy(str_span.begin(), str_span.end(), std::back_inserter(res->value));

        return res;
    }

    std::string value;
};

template <byte id> class gen_prop<std::vector<byte>, id> : public Property_id<id>
{
public:
    static std::unique_ptr<Property> from_span(span_t d)
    {
        const auto len = (d[0] << 8) + d[1];

        if (d.size() != 2 + len)
        {
            // wrong length
            return nullptr;
        }

        auto res = std::make_unique<gen_prop<std::vector<byte>, id>>();
        res->value.reserve(len);
        const auto str_span = d.subspan(2);
        std::copy(str_span.begin(), str_span.end(), std::back_inserter(res->value));

        return res;
    }

    std::string value;
};

template <byte id> class gen_prop<vbi_decoder, id> : public Property_id<id>
{
public:
    static std::unique_ptr<Property> from_span(span_t d)
    {
        vbi_decoder dec{};
        for (const auto c : d)
        {
            if(dec)
            {
                dec.read_byte(c);
            }
            else
            {
                // wrong length
                return nullptr;
            }
        }
        auto res = std::make_unique<gen_prop<vbi_decoder, id>>();
        res->value = vbi_decoder::value_type(dec);
        return res;
    }

    vbi_decoder::value_type value;
};

template <typename T> struct value_holder
{
    T value;
};

template <typename T> class span_decoder : public value_holder<byte>, public Property
{
public:
    static std::unique_ptr<Property> from_span(span_t d);
};

template <> class span_decoder<byte> : public value_holder<byte>, public Property
{
public:
    static std::unique_ptr<Property> from_span(span_t d)
    {
        if (d.size() != 1)
        {
            return nullptr;
        }

        auto res = std::make_unique<span_decoder<byte>>();
        res->value = d[0];
        return res;
    }
};

template <byte id> struct id_holder
{
    constexpr static byte identifier = id;
};

template <typename T, int id> class s_property : public span_decoder<T>, public id_holder<id>
{};


// Packet specifications
typedef s_property<byte, 0x01> Payload_Format_Indicator;
typedef s_property<uint32_t, 0x02> Message_Expiry_Interval;
typedef gen_prop<std::string, 0x03> Content_Type;
typedef gen_prop<std::string, 0x08> Response_Topic;
typedef gen_prop<std::vector<byte>, 0x09> Correlation_Data;
typedef gen_prop<vbi_decoder, 0x0B> Subscription_Identifier;
typedef gen_prop<byte, 0x24> Maximum_QoS;

} // namespace properties

std::unique_ptr<Property> get_prop(span_t);

}

#endif //MIKADO_PROPERTIES_H_INCLUDED
#endif //0