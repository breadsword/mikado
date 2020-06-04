#ifndef MIKADO_UTILS_H
#define MIKADO_UTILS_H

#include <gsl-lite/gsl-lite.hpp>

namespace mikado {

typedef uint8_t byte;

constexpr byte msb(uint16_t n)
{
    return gsl::narrow_cast<byte>(n >> 8);
}

constexpr byte lsb(uint16_t n)
{
    return gsl::narrow_cast<byte>( n & (0xFF));
}

template <typename vbi_t>
class vbi_range
{
public:

    class encoding_iterator{
    public:
        using iterator_category = std::input_iterator_tag;
        using value_type = vbi_t;
        using difference_type = void;
        using pointer = vbi_t*;
        using reference = vbi_t&;

        encoding_iterator(const value_type _value=0) : val{_value}, isEnd{false}
        {}

        byte operator*() const
        {
            byte res = (val % 128);
            if (val >= 128)
            {
                res |= byte{128};
            }

            return res;
        }

        // preincrement
        encoding_iterator& operator++()
        {
            val /= 128;
            if (val == 0){
                isEnd = true;
            }
            return *this;
        }

        // postincrement
        encoding_iterator operator++(int)
        {
            encoding_iterator temp(*this);
            ++*this;
            return temp;
        }

        explicit operator bool() const
        {
            return !isEnd;
        }

        // Equality is only true for end() iterators
        friend
        bool operator==(const encoding_iterator& lhs, const encoding_iterator& rhs)
        {
            return lhs.isEnd && rhs.isEnd;
        }

        friend
        bool operator!=(const encoding_iterator& lhs, const encoding_iterator& rhs)
        {
            return !(lhs == rhs);
        }

    private:
        value_type val;
        bool isEnd;
    };

    vbi_range<vbi_t>(const typename encoding_iterator::value_type val) : it(val)
    {}

    encoding_iterator begin() const
    {
        return it;
    }

    static const encoding_iterator end()
    {
        encoding_iterator it(0);
        return ++it;
    }

private:
    encoding_iterator it;
};

typedef vbi_range<uint16_t> vbi;

} // namespace mikado

#endif //MIKADO_UTILS_H
