#include "geo2/map_obj/projectile_type1/basic_proj_1.h"
#include "geo2/map_obj/unit_type1/unit_type1.h"
#include "geo2/map_obj/map_obj_args.h"

namespace geo2 { namespace map_obj {

BasicProj_1::BasicProj_1(const std::shared_ptr<MapObject> &owner_,
                         double damage_,
                         double lifespan_,
                         MapCoord pos_,
                         MapVec velocity_):
    Projectile_Type1(owner_),
    damage(damage_),
    life_left(lifespan_),
    current_position(pos_),
    velocity(velocity_)
{}
void BasicProj_1::run1_mt(const MapObjRun1Args &args)
{
    life_left -= args.tick_len;

    if(life_left < 0) {
        alive_status.set_status_to_dead();
        args.delete_me();
        return;
    }

    desired_position = current_position + velocity*args.tick_len;

    auto cur = args.get_sole_current_pos();
    copy_base_shape_into(cur);
    cur->translate(current_position.x, current_position.y);
    auto des = args.get_sole_desired_pos();
    copy_base_shape_into(des);
    des->translate(desired_position.x, desired_position.y);

    args.set_move_intent(MoveIntent::GoToDesiredPos);
}
void BasicProj_1::run3_mt([[maybe_unused]] const MapObjRun3Args &args)
{
    if(args.get_move_intent() == MoveIntent::GoToDesiredPos) {
        current_position = desired_position;
    }
}
double BasicProj_1::get_damage() const
{
    return damage;
}

}}
