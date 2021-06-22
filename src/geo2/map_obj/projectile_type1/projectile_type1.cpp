#include "geo2/map_obj/projectile_type1/projectile_type1.h"
#include "geo2/map_obj/unit_type1/unit_type1.h"

namespace geo2 { namespace map_obj {

Projectile_Type1::Projectile_Type1(const std::shared_ptr<Unit_Type1> &owner_,
                                   double lifespan_,
                                   MapCoord pos_,
                                   MapVec velocity_):
    owner(owner_),
    life_left(lifespan_),
    cur_pos(pos_),
    velocity(velocity_),
    team(owner_->get_team())
{

}

void Projectile_Type1::run1_mt(const MapObjRun1Args &args)
{
    life_left -= args.tick_len;

    if(life_left < 0) {
        args.set_move_intent(MoveIntent::Delete);
        return;
    }

    move_intent = MoveIntent::GoToDesiredPos;
    args.set_move_intent(MoveIntent::GoToDesiredPos);
}
MoveIntent Projectile_Type1::handle_collision(MapObject *other, const HandleCollisionArgs &args) const
{
    return other->handle_collision(*this, args);
}
MoveIntent Projectile_Type1::handle_collision([[maybe_unused]] const Wall_Type1 &other,
                                              [[maybe_unused]] const HandleCollisionArgs &args)
{
    return MoveIntent::Delete;
}
MoveIntent Projectile_Type1::handle_collision(const Unit_Type1 &other,
                                              [[maybe_unused]] const HandleCollisionArgs &args)
{
    if(are_teams_enemies(team, other.get_team())) {
        return MoveIntent::Delete;
    }
    return MoveIntent::GoToDesiredPos;
}
MoveIntent Projectile_Type1::handle_collision([[maybe_unused]] const Projectile_Type1 &other,
                                              [[maybe_unused]] const HandleCollisionArgs &args)
{
    return MoveIntent::GoToDesiredPos;
}

}}
