#pragma once

#include "geo2/map_obj/map_object.h"
#include "geo2/ceng1_data.h"

namespace geo2 {

enum class LevelName {
    NotSet,
    Test1,
    Test2,
    Test3,
};

struct Level
{
    std::vector<std::shared_ptr<map_obj::MapObject>> map_objs;
    bool is_timed;
    double time_limit;
    double player_start_x;
    double player_start_y;
};

}
