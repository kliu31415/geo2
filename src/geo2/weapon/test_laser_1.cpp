#include "geo2/weapon/test_laser_1.h"

namespace geo2 { namespace weapon {

constexpr double FIRE_INTERVAL = 0.2;

void TestLaser1::run(const WeaponRunArgs &args)
{
    if(reload_counter > 0)
        reload_counter -= args.tick_len;

    if(reload_counter<=0 && args.is_lmb_down()) {
        //
        //args.add_map_obj(proj);
        reload_counter += FIRE_INTERVAL;
    }
}

}}
