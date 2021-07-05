#pragma once

#include "geo2/rng.h"

namespace geo2 {

class RNG_Args
{
    Xorshift64RNG *rng;
public:
    Xorshift64RNG *get_rng() const
    {
        return rng;
    }
    void set_rng(Xorshift64RNG *rng_)
    {
        rng = rng_;
    }
};

}
