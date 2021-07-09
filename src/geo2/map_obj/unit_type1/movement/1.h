#pragma once

#include "geo2/geometry.h"
#include "geo2/rng.h"

#include "geo2/map_obj/map_obj_args.h"

#include <cstdint>
#include <random>
#include <type_traits>

namespace geo2 { namespace map_obj { namespace unit_movement

template<class T, class std::enable_if_t<std::is_base_of<Unit_Type1, T>>>
class _1
{
    enum class State {
        NotSet,
        MovingNormally,
        BackingUp
    };
    State current_state;
    void recalc_moving_normally()
    {

        *desired_velocity =
    }
protected:
    StandardRNG *rng;
    MapCoord *current_position;
    MapCoord *desired_position;
    MapVec *current_velocity;
    MapVec *desired_velocity;
    double max_speed;
    double accel;
    double *current_angle;
    double *desired_angle;
    double cur_level_time;
    double tick_len;

    void run()
    {
        switch(*current_state) {
        case State::NotSet:
            *current_state = State::MovingNormally;
            recalc_moving_normally();
            [[fallthrough]];
        case State::MovingNormally:

            break;
        case State::BackingUp:
            break;
        }
    }

    virtual double get_max_speed() = 0;
};

}}
