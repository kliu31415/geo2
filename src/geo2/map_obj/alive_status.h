#pragma once

#include "kx/debug.h"

namespace geo2 { namespace map_obj {

class AliveStatus
{
    enum class State {Alive, JustDied, Dead};
    State state;
public:
    AliveStatus():
        state(State::Alive)
    {}
    bool is_dead() const
    {
        return state == State::Dead;
    }
    void end_current_time_block()
    {
        if(state == State::JustDied)
            state = State::Dead;
    }
    void set_status_to_just_died()
    {
        k_expects(state == State::Alive);
        state = State::JustDied;
    }
    void set_status_to_dead()
    {
        k_expects(state == State::Alive);
        state = State::Dead;
    }
};

}}
