#if 0
#include "properties.h"

using namespace mikado::properties;

template <> std::unique_ptr<mikado::Property> span_decoder<uint32_t>::from_span(span_t d)
{
    if (d.size() != 4)
    {
        return nullptr;
    }

    auto res = std::make_unique<span_decoder<uint32_t>>();
    res->value = (d[0] << 24) + (d[1] << 16) + (d[2] << 8) + d[3];
    return res;
}


std::unique_ptr<mikado::Property> mikado::get_prop(span_t s)
{
    const byte identifier(s[0]);
    const auto d = s.subspan(1);

    switch(identifier){
    case Payload_Format_Indicator::identifier:
        return Payload_Format_Indicator::from_span(d);
    case Message_Expiry_Interval::identifier:
        return Message_Expiry_Interval::from_span(d);

    case Content_Type::identifier:
        return Content_Type::from_span(d);
    case Response_Topic::identifier:
        return Response_Topic::from_span(d);
    case Correlation_Data::identifier:
        return Correlation_Data::from_span(d);
    case Subscription_Identifier::identifier:
        return Subscription_Identifier::from_span(d);


    case Maximum_QoS::identifier:
        return Maximum_QoS::from_span(d);

    default:
        return nullptr;
    }
}

#endif //0
