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
class vbi_encoder
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
            val >>= 7;
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

    vbi_encoder<vbi_t>(const typename encoding_iterator::value_type val) : value{val}
    {}

    encoding_iterator begin() const
    {
        return encoding_iterator(value);
    }

    static const encoding_iterator end()
    {
        encoding_iterator it(0);
        return ++it;
    }

private:
    const typename encoding_iterator::value_type value;
};

typedef vbi_encoder<uint16_t> vbi;

/**
 * @brief The vbi_decoder class
 *
 * I thought about modelling this as an output iterator. However, the input will
 * be gathered byte-by-byte from a network stream, so it is natural to call de-
 * coding with a function byte-by-byte and check if more data is needed.
 * An output iterator would make sense, if the input sequence was available and
 * allow to std::copy() the data.
 */
class vbi_decoder{
public:
    typedef uint16_t value_type;

    vbi_decoder() : value{0}, multiplier{0}, more_to_read{true}
    {}

    bool read_byte(byte b);

    explicit operator value_type()
    {
        return value;
    }

    explicit operator bool()
    {
        return more_to_read;
    }

private:
    value_type value, multiplier;
    bool more_to_read;
};


} // namespace mikado

#endif //MIKADO_UTILS_H
