#pragma once

#include "geo2/rng.h"

namespace geo2 {

class RNG_Args
{
    StandardRNG *rng;
public:
    StandardRNG *get_rng() const
    {
        return rng;
    }
    void set_rng(StandardRNG *rng_)
    {
        rng = rng_;
    }
};

}
