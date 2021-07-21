#pragma once

#include "geo2/geometry.h"

#include <cstdint>

namespace geo2 {

struct CEng1Collision final
{
    int idx1;
    int idx2;
    void swap()
    {
        std::swap(idx1, idx2);
    }
};

}
