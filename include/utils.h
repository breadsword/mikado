#ifndef MIKADO_UTILS_H
#define MIKADO_UTILS_H

#include <gsl-lite/gsl-lite.hpp>

namespace mikado {

typedef uint8_t byte;

/** \brief Copy command bound on output range size
 *
 * Needed, as we use spans as output range for recv. This way we can copy data
 * left vs right
 *
 * TODO: implement memmove() for known lengths
 *
 * taken from https://stackoverflow.com/questions/8432864/strncpy-equivalent-of-stdcopy
 */
template<class InputIterator , class OutputIterator>
size_t copy( InputIterator srcStart , InputIterator srcEnd ,
             OutputIterator destStart , OutputIterator destEnd )
{
    size_t items_copied = 0;
    while ( srcStart != srcEnd && destStart != destEnd )
    {
        *destStart = *srcStart;
        ++srcStart;
        ++destStart;
        ++items_copied;
    }
    return items_copied;
}

/** \brief Shorthand for ranges (i.e. spans/arrays/vectors)
 *
 */
template <class InputRange, class OutputRange>
size_t copy( InputRange in, OutputRange out)
{
    return copy (std::begin(in), std::end(in), std::begin(out), std::end(out));
}

constexpr byte msb(uint16_t n)
{
    return gsl::narrow_cast<byte>(n >> 8);
}

constexpr byte lsb(uint16_t n)
{
    return gsl::narrow_cast<byte>( n & (0xFF));
}

} // namespace mikado

#endif //MIKADO_UTILS_H
