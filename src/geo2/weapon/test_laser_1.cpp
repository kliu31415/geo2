#include "geo2/weapon/test_laser_1.h"
#include "geo2/map_obj/projectile_type1/laser_proj_1.h"

namespace geo2 { namespace weapon {

TestLaser1::TestLaser1(const std::shared_ptr<map_obj::MapObject> &owner_):
    Weapon(owner_)
{

}

constexpr double FIRE_INTERVAL = 0.001;
constexpr double LIFESPAN = 1.0;

void TestLaser1::run(const WeaponRunArgs &args)
{
    if(reload_counter > 0)
        reload_counter -= args.get_tick_len();

    if(reload_counter<=0 && args.is_lmb_down()) {
        auto mobj = owner.lock();
        k_expects(mobj != nullptr);
        auto owner_info = args.get_owner_info();

        auto proj = std::make_unique<map_obj::LaserProj_1>(mobj,
                                                           LIFESPAN,
                                                           owner_info.pos,
                                                           MapVec(100, 0),
                                                           100*args.get_cur_level_time());
        args.add_map_obj(std::move(proj));
        reload_counter += FIRE_INTERVAL;
    }
}

}}
