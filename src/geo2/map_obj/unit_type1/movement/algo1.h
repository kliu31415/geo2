#pragma once

#include "geo2/geometry.h"
#include "geo2/rng.h"
#include "geo2/map_obj/unit_type1/unit_type1.h"

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
};

class Algo1
{
    enum class State {
        NotSet,
        Rotating,
        Translating,
        Resting
    };

    MapVec current_velocity;
    MapVec translation_accel;

    double accel;
    double current_speed;
    double desired_speed;
    double max_speed;

    double current_d_angle;
    double desired_d_angle;
    double angular_accel;

    State current_state;

    double resting_until;

    void calc_translating_movement(StandardRNG *rng, double max_accel, double max_speed_)
    {
        k_expects(current_speed == 0);
        k_expects(current_velocity == MapVec(0, 0));

        current_state = State::Translating;
        accel = max_accel;

        if(std::uniform_int_distribution<int>(0, 2)(*rng) == 0) {
            translation_accel = accel * MapVec(std::cos(current_angle), std::sin(current_angle));
            max_speed = max_speed_;
        } else {
            constexpr double MOVE_BACK_PENALTY = 0.5;
            translation_accel = -MOVE_BACK_PENALTY*accel * MapVec(std::cos(current_angle), std::sin(current_angle));
            max_speed = MOVE_BACK_PENALTY*max_speed_;
        }

        max_speed *= std::uniform_real_distribution<double>(0.7, 1)(*rng);
    }
    void calc_rotating_movement(StandardRNG *rng, [[maybe_unused]] double max_angular_accel)
    {
        k_expects(current_d_angle == 0);

        current_state = State::Rotating;

        do angular_accel = std::uniform_real_distribution<double>(-1, 1)(*rng);
        while(std::fabs(angular_accel) < 0.6);

        angular_accel *= max_angular_accel;
    }
    void calc_resting_movement(StandardRNG *rng, double cur_level_time)
    {
        current_state = State::Resting;
        resting_until = cur_level_time + std::max(0.0, std::uniform_real_distribution<double>(-1, 0.1)(*rng));
    }
public:
    MapCoord desired_position;
    double current_angle;
    double desired_angle;

    Algo1():
        current_velocity(0, 0),
        current_speed(0),
        current_d_angle(0),
        current_state(State::NotSet)
    {}
    virtual ~Algo1() = default;

    void movement_algo_run(const Algo1RunArgs &args)
    {
        switch(current_state) {
        case State::NotSet:
            current_state = State::Translating;
            calc_translating_movement(args.rng, args.max_accel, args.max_speed);
            [[fallthrough]];
        case State::Translating:
            desired_position = *args.current_position + args.tick_len * current_velocity;
            desired_speed = std::min(current_speed + args.tick_len * accel, max_speed);
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
            current_speed = std::max(current_speed - args.tick_len * args.max_accel, 0.0);
            if(current_speed > 1e-9)
                current_velocity = current_velocity / current_velocity.norm() * current_speed;
            else
                current_velocity = MapVec(0, 0);

            desired_angle = current_angle + args.tick_len * current_d_angle;
            if(current_d_angle > 0)
                current_d_angle = std::max(current_d_angle - args.tick_len * args.max_angular_accwel, 0.0);
            else
                current_d_angle = std::min(current_d_angle + args.tick_len * args.max_angular_accel, 0.0);
            break;
        }
    }
    void movement_algo_handle_successful_movement(const Algo1HandleMovementArgs &args)
    {
        switch(current_state) {
        case State::NotSet:
            k_assert(false);
            break;
        case State::Translating:
            current_speed = desired_speed;
            current_velocity += args.tick_len * translation_accel;
            *args.current_position = desired_position;
            if(std::uniform_real_distribution<double>(0, 0.25)(*args.rng) < args.tick_len)
                calc_resting_movement(args.rng, args.cur_level_time);
            break;
        case State::Rotating:
            current_d_angle = desired_d_angle;
            current_angle = desired_angle;
            *args.current_position = desired_position;
            if(std::uniform_real_distribution<double>(0, 0.15)(*args.rng) < args.tick_len)
                calc_resting_movement(args.rng, args.cur_level_time);
            break;
        case State::Resting:
            *args.current_position = desired_position;
            current_angle = desired_angle;
            if(args.cur_level_time >= resting_until && current_speed==0 && current_d_angle==0) {
                if(std::uniform_int_distribution<int>(0, 2)(*args.rng) == 0)
                    calc_translating_movement(args.rng, args.max_accel, args.max_speed);
                else
                    calc_rotating_movement(args.rng, args.max_angular_accel);
            }
            break;
        }
    }
    void movement_algo_handle_unsuccessful_movement(const Algo1HandleMovementArgs &args)
    {
        calc_resting_movement(args.rng, args.cur_level_time);
    }
};

}}}
