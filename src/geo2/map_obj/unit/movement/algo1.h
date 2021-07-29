#pragma once

#include "geo2/rng.h"
#include "geo2/map_obj/unit/unit.h"

#include <random>

namespace geo2 { namespace map_obj { namespace unit_movement {

struct Algo1RunArgs
{
    StandardRNG *rng;
    MapCoord *current_position;
    double max_speed;
    double max_angular_speed;
    double max_accel;
    double max_angular_accel;
    double tick_len;
    double cur_level_time;
};

struct Algo1HandleMovementArgs
{
    StandardRNG *rng;
    MapCoord *current_position;
    double max_speed;
    double max_angular_speed;
    double max_accel;
    double max_angular_accel;
    double tick_len;
    double cur_level_time;

    double translating_time_param;
    double rotating_time_param;
    double resting_time_param;
};

class Algo1 final
{
    enum class State {
        NotSet,
        Rotating,
        Translating,
        Resting
    };

    MapVec current_velocity;
    MapCoord desired_position;
    MapVec accel_vec;

    double accel;
    double current_speed;
    double desired_speed;
    double max_speed;

    double current_angle;
    double desired_angle;
    double current_d_angle;
    double desired_d_angle;
    double angular_accel;

    State current_state;

    void calc_translating_movement(StandardRNG *rng, double max_accel, double max_speed_);
    void calc_rotating_movement(StandardRNG *rng, [[maybe_unused]] double max_angular_accel);
    void calc_resting_movement();
public:
    Algo1();
    virtual ~Algo1() = default;

    void run(const Algo1RunArgs &args);
    void handle_successful_movement(const Algo1HandleMovementArgs &args);
    void handle_unsuccessful_movement([[maybe_unused]] const Algo1HandleMovementArgs &args);
    const MapCoord& get_desired_position() const;
    double get_current_angle() const;
    double get_desired_angle() const;
};

}}}
