#pragma once

#include <vector>

namespace geo2 { namespace level_gen { namespace v1 {

struct Grid
{
    size_t rows;
    size_t cols;
    std::vector<bool> vals;
};

using room_gen_id_t = int;

class LevelGenerator;

class RoomGenerator
{
    room_gen_id_t id;
protected:
    RoomGenerator(LevelGenerator *level_generator);
public:
    virtual ~RoomGenerator() = default;
    room_gen_id_t get_id() const;
    virtual Grid get_occupied_tiles() = 0;
};

class RoomGenerator1 final: public RoomGenerator
{
public:
    RoomGenerator1(LevelGenerator *level_generator);
    Grid get_occupied_tiles() override;
};

class LevelGenerator
{
    room_gen_id_t room_gen_id_counter;
public:
    LevelGenerator();
    room_gen_id_t get_new_room_gen_id(kx::Passkey<RoomGenerator>);
};

}}}
