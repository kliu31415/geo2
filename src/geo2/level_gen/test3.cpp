#include "geo2/level_gen/test3.h"

#include "geo2/map_obj/floor_type1/monochromatic_floor_1.h"
#include "geo2/map_obj/wall_type1/monochromatic_wall_1.h"
#include "geo2/map_obj/unit/pig_1.h"
#include "geo2/map_obj/unit/spotted_pig_1.h"
#include "geo2/map_obj/unit/hexfly_1.h"

namespace geo2 { namespace level_gen {

Level NamedLevelGenerator<LevelName::Test3>::generate([[maybe_unused]] Game *game)
{
    using namespace map_obj;

    Level level;

    for(int i=0; i<30; i++) {
        kx::gfx::LinearColor color(2.5f, 1.5f, 0.5f, 1.0f);
        MapRect pos(i, 0, 1, 1);
        level.map_objs.push_back(std::make_shared<MonochromaticWall_1>(pos, color));
        pos = MapRect(i, 30, 1, 1);
        level.map_objs.push_back(std::make_shared<MonochromaticWall_1>(pos, color));
        if(i != 0) {
            pos = MapRect(0, i, 1, 1);
            level.map_objs.push_back(std::make_shared<MonochromaticWall_1>(pos, color));
            pos = MapRect(29, i, 1, 1);
            level.map_objs.push_back(std::make_shared<MonochromaticWall_1>(pos, color));
        }
    }

    for(int i=1; i<29; i++) {
        for(int j=1; j<30; j++) {
            kx::gfx::LinearColor color(0.5f, 0.6f, 0.7f, 1.0f);
            MapRect pos(i, j, 1, 1);
            level.map_objs.push_back(std::make_shared<MonochromaticFloor_1>(pos, color));
        }
    }
    auto add_func = [&level] (std::shared_ptr<MapObject> &&map_obj) -> void
                                    {
                                        level.map_objs.push_back(std::move(map_obj));
                                    };
    for(int i=0; i<3; i++) {
        for(int j=0; j<3; j++) {
            if(i == 1)
                Spotted_Pig_1::make_standard(add_func, MapCoord(6 + 5*i, 6 + 5*j));
            else if(i == 2)
                Pig_1::make_standard(add_func, MapCoord(6 + 5*i, 6 + 5*j));
            else
                Hexfly_1::make_standard(add_func, MapCoord(6 + 5*i, 6 + 5*j));
        }
    }

    level.time_to_complete = 120;
    level.player_start_x = 4;
    level.player_start_y = 4;
    return level;
}

}}
