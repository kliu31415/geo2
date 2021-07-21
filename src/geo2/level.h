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
    std::vector<std::shared_ptr<map_obj::MapObject>> to_add;
    std::vector<std::shared_ptr<map_obj::MapObject>> map_objs;
    std::vector<CEng1Data> ceng_data;
    double time_to_complete;
    double player_start_x;
    double player_start_y;
};

}
