#define BOOST_TEST_MODULE vbi test
#include <boost/test/unit_test.hpp>

#include "utils.h"

#include <algorithm>


using namespace mikado;


BOOST_AUTO_TEST_CASE( encode_zero )
{
    const int i = 0;

    auto enc = vbi(i);
    BOOST_CHECK_EQUAL(*enc.begin(), byte{0});
    BOOST_CHECK(++enc.begin() == enc.end());
}

BOOST_AUTO_TEST_CASE( encode_std_copy )
{
    auto enc = vbi(42);

    std::vector<mikado::byte> out;
    std::copy(enc.begin(), enc.end(), std::back_inserter(out));

    BOOST_CHECK_EQUAL(out[0], 42);
    BOOST_CHECK_EQUAL(out.size(), 1);
}

BOOST_AUTO_TEST_CASE( encode_two_digits )
{
    {
        const auto ref = {0x80, 0x01};
        BOOST_CHECK_EQUAL_COLLECTIONS(vbi(128).begin(), vbi::end(),
                                      std::begin(ref), std::end(ref));
    }

    {
        const auto ref = {0xFF, 0x7F};
        BOOST_CHECK_EQUAL_COLLECTIONS(vbi(16383).begin(), vbi::end(),
                                      std::begin(ref), std::end(ref));
    }
}

BOOST_AUTO_TEST_CASE( encode_three_four_digits )
{

    {
        const auto ref = {0x80, 0x80, 0x01};
        BOOST_CHECK_EQUAL_COLLECTIONS(vbi(16384).begin(), vbi::end(),
                                      std::begin(ref), std::end(ref));
    }
    typedef vbi_range<uint32_t> vbil;
    {
        const auto ref = {0xFF, 0xFF, 0x7F};
        BOOST_CHECK_EQUAL_COLLECTIONS(vbil(2097151).begin(), vbil::end(),
                                      std::begin(ref), std::end(ref));
    }
    {
        const auto ref = {0x80, 0x80, 0x80, 0x01};
        BOOST_CHECK_EQUAL_COLLECTIONS(vbil(2097152).begin(), vbil::end(),
                                      std::begin(ref), std::end(ref));
    }
    {
        const auto ref = {0xFF, 0xFF, 0xFF, 0x7F};
        BOOST_CHECK_EQUAL_COLLECTIONS(vbil(268435455).begin(), vbil::end(),
                                      std::begin(ref), std::end(ref));
    }

}
