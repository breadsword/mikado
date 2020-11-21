#define BOOST_TEST_MODULE mikado_properties test
#include <boost/test/unit_test.hpp>

#include <gsl-lite/gsl-lite.hpp>

#include "properties.h"

namespace m = mikado;


BOOST_AUTO_TEST_CASE ( decode_wrong_byte )
{
    {
        // wrong identifier
        constexpr m::byte ref[] = { 0x00, 42 };
        BOOST_REQUIRE_EQUAL(m::get_prop(ref), nullptr);
    }

    {
        // too short (check that no out-of-bounds access happens
        constexpr m::byte ref[] = { 0x01 };
        BOOST_REQUIRE_EQUAL(m::get_prop(ref), nullptr);
    }

    {
        // too long
        constexpr m::byte ref[] = { 0x01, 1, 2 };
        BOOST_REQUIRE_EQUAL(m::get_prop(ref), nullptr);
    }
}

BOOST_AUTO_TEST_CASE ( decode_templated_byte )
{
    const m::byte ref[] = {0x01, 123 };

    auto prop = m::get_prop(ref);
    auto p = dynamic_cast<m::properties::Maximum_QoS *>(prop.get());
    BOOST_REQUIRE_NE(p, nullptr);
    BOOST_CHECK_EQUAL(p->value, 123);
}

BOOST_AUTO_TEST_CASE ( decode_integer )
{
    const m::byte ref[] = {0x02, 1, 2, 3, 4};
    auto prop = m::get_prop(ref);
    auto p = dynamic_cast<m::properties::Message_Expiry_Interval *>(prop.get());

    BOOST_REQUIRE_NE(p, nullptr);
    BOOST_CHECK_EQUAL(p->value, (1<<24) + (2 << 16) + (3 << 8) + 4);

}

BOOST_AUTO_TEST_CASE( decode_string )
{
    {
        const m::byte ref[] = {0x03, 0, 4, 'l', 'a', 'b', 'c'};
        auto prop = m::get_prop(ref);
        auto p = dynamic_cast<m::properties::Content_Type *>(prop.get());

        BOOST_REQUIRE_NE(p, nullptr);
        BOOST_CHECK_EQUAL(p->value, "labc");
    }
    {
        // wrong string length
        const m::byte ref[] = {0x03, 0, 5, 'l', 'a', 'b', 'c'};
        BOOST_REQUIRE_EQUAL(m::get_prop(ref), nullptr);
    }
}

BOOST_AUTO_TEST_CASE( decode_vbi )
{
    {
        const m::byte ref[] = {0x0B, 0x80, 0x01};
        auto prop = m::get_prop(ref);
        auto p = dynamic_cast<m::properties::Subscription_Identifier *>(prop.get());

        BOOST_REQUIRE_NE(p, nullptr);
        BOOST_CHECK_EQUAL(p->value, 128);
    }
}
