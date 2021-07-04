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

void BasicProj_1::run2_st([[maybe_unused]] const MapObjRun2Args &args)
{
    if(args.get_move_intent() == MoveIntent::GoToDesiredPos) {
        current_position = desired_position;
    }
}

}}
