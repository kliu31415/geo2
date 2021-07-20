#include "geo2/rng.h"

#include <intrin.h>

namespace geo2 {

Xorshift64RNG::Xorshift64RNG()
{
    x = __rdtsc();
    x |= 0x1;
}

}
