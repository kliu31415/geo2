#include "geo2/level_gen/test3.h"

#include "geo2/map_obj/floor_type1/monochromatic_floor_1.h"
#include "geo2/map_obj/wall_type1/monochromatic_wall_1.h"
#include "geo2/map_obj/unit_type1/pig_1.h"

namespace geo2 {

Level LevelGenerator<Level::Name::Test3>::generate([[maybe_unused]] Game *game)
{
    using namespace map_obj;

    Level level;

    for(int i=0; i<20; i++) {
        kx::gfx::LinearColor color(2.5f, 1.5f, 0.5f, 1.0f);
        MapRect pos(i, 0, 1, 1);
        level.map_objs.push_back(std::make_shared<MonochromaticWall_1>(pos, color));
        pos = MapRect(i, 20, 1, 1);
        level.map_objs.push_back(std::make_shared<MonochromaticWall_1>(pos, color));
        if(i != 0) {
            pos = MapRect(0, i, 1, 1);
            level.map_objs.push_back(std::make_shared<MonochromaticWall_1>(pos, color));
            pos = MapRect(19, i, 1, 1);
            level.map_objs.push_back(std::make_shared<MonochromaticWall_1>(pos, color));
        }
    }

    for(int i=1; i<19; i++) {
        for(int j=1; j<20; j++) {
            kx::gfx::LinearColor color(0.5f, 0.6f, 0.7f, 1.0f);
            MapRect pos(i, j, 1, 1);
            level.map_objs.push_back(std::make_shared<MonochromaticFloor_1>(pos, color));
        }
    }

    level.map_objs.push_back(std::make_shared<Pig_1>(MapCoord(8.0f, 8.0f)));

    level.time_to_complete = 120;
    level.player_start_x = 4;
    level.player_start_y = 4;
    return level;
}

}
