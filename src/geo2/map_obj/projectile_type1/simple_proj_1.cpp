#include "geo2/map_obj/projectile_type1/simple_proj_1.h"
#include "geo2/map_obj/unit_type1/unit_type1.h"

namespace geo2 { namespace map_obj {

SimpleProj_1::SimpleProj_1(const std::shared_ptr<Unit_Type1> &owner_,
                           double lifespan_,
                           MapCoord pos_,
                           MapVec velocity_,
                           std::unique_ptr<Polygon> &&shape):
    Projectile_Type1(owner_),
    life_left(lifespan_),
    cur_pos(pos_),
    velocity(velocity_)
{
    cur_shape = std::move(shape);
}

void SimpleProj_1::run2_st([[maybe_unused]] const MapObjRun2Args &args)
{
    if(move_intent == MoveIntent::GoToDesiredPos) {
        cur_pos = desired_pos;
    }
}

}}
