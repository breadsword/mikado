#include "mikado.h"

#include "utils.h"
#include "packets.h"

namespace mikado{

std::unique_ptr<Packet> Packet::parse(gsl::span<byte> data)
{
    return nullptr;
}

}
