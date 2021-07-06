#pragma once

#include "geo2/geometry.h"
#include "geo2/rng.h"

#include <cstdint>
#include <random>

namespace geo2 { namespace map_obj { namespace unit_move

struct CardinalRotation_1
{
    Xorshift64RNG *rng;
    double tick_len;
    MapCoord *current_position;
    MapCoord *desired_position;
    MapVec *current_velocity;
    MapVec *desired_velocity;
    double *current_angle;
    double *desired_angle;

    void run()
    {

    }
};

}}
