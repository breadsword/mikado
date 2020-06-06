#include "utils.h"


bool mikado::vbi_decoder::read_byte(mikado::byte b)
{
    value += ((b & 0x7F) << (7*multiplier));
    multiplier ++;

    more_to_read = (b >> 7);

    return multiplier < 4;
}
