#pragma once

#include <vector>

namespace geo2 { namespace level_gen { namespace v1 {

class RoomGen
{

};

using room_gen_set_id_t = uint16_t;

struct RoomGenSet
{
    room_gen_set_id_t id;
    std::vector<RoomGen> room_gens;

};

class LevelGen
{
    room_gen_set_id_t room_gen_set_id;
public:
    LevelGen();
    RoomGenSet make_room_gen_set();

};

}}}
