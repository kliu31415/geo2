#include "geo2/map_obj/unit_type1/movement/algo1.h"
#include "geo2/geometry.h"

#include <random>

namespace geo2 { namespace map_obj { namespace unit_movement {

using std::uniform_real_distribution;
using std::uniform_int_distribution;
using std::sin;
using std::cos;
using std::min;
using std::max;

void Algo1::calc_translating_movement(StandardRNG *rng, double max_accel, double max_speed_)
{
    k_expects(current_speed == 0);
    k_expects(current_velocity == MapVec(0, 0));

    current_state = State::Translating;
    accel = max_accel;

    if(uniform_int_distribution<int>(0, 2)(*rng) == 0) {
        accel_vec = accel * MapVec(cos(current_angle), sin(current_angle));
        max_speed = max_speed_;
    } else {
        constexpr double MOVE_BACK_PENALTY = 0.5;
        accel_vec = -MOVE_BACK_PENALTY*accel * MapVec(cos(current_angle), sin(current_angle));
        max_speed = MOVE_BACK_PENALTY*max_speed_;
    }

    max_speed *= uniform_real_distribution<double>(0.7, 1)(*rng);
}
void Algo1::calc_rotating_movement(StandardRNG *rng, [[maybe_unused]] double max_angular_accel)
{
    k_expects(current_d_angle == 0);

    current_state = State::Rotating;

    do angular_accel = uniform_real_distribution<double>(-1, 1)(*rng);
    while(std::fabs(angular_accel) < 0.6);

    angular_accel *= max_angular_accel;
}
void Algo1::calc_resting_movement()
{
    current_state = State::Resting;
}

Algo1::Algo1():
    current_velocity(0, 0),
    current_speed(0),
    current_d_angle(0),
    current_state(State::NotSet)
{}

void Algo1::run(const Algo1RunArgs &args)
{
    switch(current_state) {
    case State::NotSet:
        current_state = State::Translating;
        calc_translating_movement(args.rng, args.max_accel, args.max_speed);
        [[fallthrough]];
    case State::Translating:
        desired_position = *args.current_position + args.tick_len * current_velocity;
        desired_speed = min(current_speed + args.tick_len * accel, max_speed);
        desired_angle = current_angle;
        break;
    case State::Rotating:
        desired_position = *args.current_position;
        desired_angle = current_angle + args.tick_len * current_d_angle;
        desired_d_angle = std::clamp(current_d_angle + args.tick_len * angular_accel,
                                     -args.max_angular_speed,
                                      args.max_angular_speed);
        break;
    case State::Resting:
        desired_position = *args.current_position + args.tick_len * current_velocity;
        current_speed = max(current_speed - args.tick_len * args.max_accel, 0.0);
        if(current_speed > 1e-9)
            current_velocity = current_velocity / current_velocity.norm() * current_speed;
        else
            current_velocity = MapVec(0, 0);

        desired_angle = current_angle + args.tick_len * current_d_angle;
        if(current_d_angle > 0)
            current_d_angle = std::max(current_d_angle - args.tick_len * args.max_angular_accel, 0.0);
        else
            current_d_angle = std::min(current_d_angle + args.tick_len * args.max_angular_accel, 0.0);
        break;
    }
}
void Algo1::handle_successful_movement(const Algo1HandleMovementArgs &args)
{
    switch(current_state) {
    case State::NotSet:
        k_assert(false);
        break;
    case State::Translating:
        current_speed = desired_speed;
        current_velocity += args.tick_len * accel_vec;
        *args.current_position = desired_position;
        if(uniform_real_distribution<double>(0, args.translating_time_param)(*args.rng) < args.tick_len)
            calc_resting_movement();
        break;
    case State::Rotating:
        current_d_angle = desired_d_angle;
        current_angle = desired_angle;
        *args.current_position = desired_position;
        if(uniform_real_distribution<double>(0, args.rotating_time_param)(*args.rng) < args.tick_len)
            calc_resting_movement();
        break;
    case State::Resting:
        *args.current_position = desired_position;
        current_angle = desired_angle;
        if(current_speed==0 && current_d_angle==0) {
            if(uniform_int_distribution<int>(0, args.resting_time_param)(*args.rng) == 0)
                calc_translating_movement(args.rng, args.max_accel, args.max_speed);
            else
                calc_rotating_movement(args.rng, args.max_angular_accel);
        }
        break;
    }
}
void Algo1::handle_unsuccessful_movement([[maybe_unused]] const Algo1HandleMovementArgs &args)
{
    calc_resting_movement();
}
const MapCoord& Algo1::get_desired_position() const
{
    return desired_position;
}
double Algo1::get_current_angle() const
{
    return current_angle;
}
double Algo1::get_desired_angle() const
{
    return desired_angle;
}

}}}
