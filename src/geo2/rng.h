#pragma once

#include <intrin.h>
#include <cstdint>

namespace geo2 {

class Xorshift64RNG final
{
    uint64_t x;
public:
    Xorshift64RNG()
    {
        //0x1 to prevent a seed of 0
        x = __rdtsc() | 0x1;
    }
    //Marsaglia's xorshift
    inline uint64_t operator()()
    {
        x ^= x << 13;
        x ^= x >> 7;
        x ^= x << 17;
        return x;
    }
};

}
