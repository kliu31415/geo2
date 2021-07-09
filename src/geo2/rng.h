#pragma once

#include <intrin.h>
#include <immintrin.h>
#include <cstdint>

namespace geo2 {

class Xorshift64RNG final
{
    uint64_t x;
public:
    static constexpr uint64_t min()
    {
        return 1;
    }
    static constexpr uint64_t max()
    {
        return std::numeric_limits<uint64_t>::max();
    }
    Xorshift64RNG()
    {
        x = 0;
        //WARNING: rdseed isn't supported by older CPUs (e.g. pre-Broadwell)
        //_rdseed64_step(&x);
        x += __rdtsc();
        //prevent a seed of 0
        x |= 0x1;
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

using StandardRNG = Xorshift64RNG;

}
