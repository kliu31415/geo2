#include "geo2/map_obj/projectile_type1/projectile_type1.h"
#include "geo2/map_obj/unit_type1/unit_type1.h"
#include "geo2/map_obj/map_obj_args.h"

namespace geo2 { namespace map_obj {

Projectile_Type1::Projectile_Type1(const std::shared_ptr<MapObject> &owner_):
    owner(owner_),
    team(owner_->get_team())
{}

void Projectile_Type1::handle_collision(MapObject *other, const HandleCollisionArgs &args) const
{
    other->handle_collision(*this, args);
}
void Projectile_Type1::handle_collision([[maybe_unused]] const Wall_Type1 &other,
                                        [[maybe_unused]] const HandleCollisionArgs &args)
{
    args.set_move_intent(MoveIntent::Delete);
}
void Projectile_Type1::handle_collision(const Unit_Type1 &other,
                                        [[maybe_unused]] const HandleCollisionArgs &args)
{
    if(are_enemies(team, other.get_team())) {
        args.set_move_intent(MoveIntent::Delete);
    }
}
void Projectile_Type1::handle_collision([[maybe_unused]] const Projectile_Type1 &other,
                                        [[maybe_unused]] const HandleCollisionArgs &args)
{

}

}}
