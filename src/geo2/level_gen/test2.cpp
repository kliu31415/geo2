#include "geo2/level_gen/test2.h"

#include "geo2/map_objs/wall_type1/monochromatic_wall1.h"

namespace geo2 {

Level LevelGenerator<Level::Name::Test2>::generate([[maybe_unused]] Game *game)
{
    using namespace map_obj;

    Level level;
    for(int i=0; i<40; i++) {
        for(int j=0; j<40; j++) {
            for(int x=0; x<3; x++) {
                for(int y=0; y<3; y++) {
                    MapRect pos(i*6 + x, j*6 + y, 1, 1);
                    kx::gfx::LinearColor color(std::sqrt(i+0.1), std::sqrt(j+0.1), x, 1.0);
                    level.map_objs.push_back(std::make_shared<MonochromaticWall_1>(pos, color));
                }
            }
        }
    }
    level.time_to_complete = 120;
    level.player_start_x = -1;
    level.player_start_y = -1;
    return level;
}

}
