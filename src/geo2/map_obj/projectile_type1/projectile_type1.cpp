#include "geo2/map_obj/projectile_type1/projectile_type1.h"
#include "geo2/map_obj/unit_type1/unit_type1.h"
#include "geo2/map_obj/map_obj_args.h"

namespace geo2 { namespace map_obj {

Projectile_Type1::Projectile_Type1(const std::shared_ptr<MapObject> &owner_):
    owner(owner_),
    team(owner_->get_team())
{
    k_expects(team != Team::NotSet);
}

void Projectile_Type1::handle_collision(MapObject *other, const HandleCollisionArgs &args)
{
    other->handle_collision(this, args);
}
void Projectile_Type1::handle_collision([[maybe_unused]] Wall_Type1 *other,
                                        [[maybe_unused]] const HandleCollisionArgs &args)
{
    if(!is_dead()) {
        alive_status.set_status_to_just_died();
        args.delete_me();
    }
}
void Projectile_Type1::handle_collision(Unit_Type1 *other,
                                        [[maybe_unused]] const HandleCollisionArgs &args)
{
    if(!other->is_dead() && !is_dead() && are_enemies(team, other->get_team())) {
        alive_status.set_status_to_just_died();
        args.delete_me();
    }
}
void Projectile_Type1::handle_collision([[maybe_unused]] Projectile_Type1 *other,
                                        [[maybe_unused]] const HandleCollisionArgs &args)
{

}
void Projectile_Type1::end_handle_collision_block([[maybe_unused]] const EndHandleCollisionBlockArgs &args)
{
    alive_status.end_current_time_block();
}
bool Projectile_Type1::is_dead() const
{
    return alive_status.is_dead();
}
Team Projectile_Type1::get_team() const
{
    return team;
}

}}
