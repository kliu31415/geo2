#pragma once

#include <limits>
#include <cstdint>

namespace geo2 {

class Xorshift64RNG final
{
    uint64_t x;
public:
    using result_type = uint64_t;

    static constexpr uint64_t min()
    {
        return 1;
    }
    static constexpr uint64_t max()
    {
        return std::numeric_limits<uint64_t>::max();
    }
    Xorshift64RNG();
    //Marsaglia's xorshift
    inline uint64_t operator()()
    {
        x ^= x << 13;
        x ^= x >> 7;
        x ^= x << 17;
        return x;
    }
};

using StandardRNG = Xorshift64RNG;

}
