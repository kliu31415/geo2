#include "geo2/level_gen/v1/level_gen1.h"

namespace geo2 { namespace level_gen { namespace v1 {

RoomGenerator::RoomGenerator(LevelGenerator *level_generator):
    id(level_generator->get_new_room_gen_id({}))
{}
room_gen_id_t RoomGenerator::get_id() const
{
    return id;
}
LevelGenerator::LevelGenerator():
    room_gen_id_counter(0)
{

}
room_gen_id_t LevelGenerator::get_new_room_gen_id(kx::Passkey<RoomGenerator>)
{
    return room_gen_id_counter++;
}

}}}
