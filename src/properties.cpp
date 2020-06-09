#include "properties.h"

using namespace mikado::properties;

std::unique_ptr<mikado::Property> mikado::get_prop(span_t d)
{
    const byte identifier = d[0];

    switch(identifier){
    case Payload_Format_Indicator::identifier:
        return Payload_Format_Indicator::from_span(d);
    case Message_Expiry_Interval::identifier:
        return Message_Expiry_Interval::from_span(d);
    case Content_Type::identifier:
        return Content_Type::from_span(d);
    case Maximum_QoS::identifier:
        return Maximum_QoS::from_span(d);
    default:
        return nullptr;
    }
}
