#include "geo2/weapon/weapon.h"

namespace geo2 { namespace weapon {

Weapon::Weapon(const std::shared_ptr<map_obj::MapObject> &owner_):
    owner(owner_)
{}
void Weapon::swap_in([[maybe_unused]] const WeaponSwapInArgs &args)
{

}
void Weapon::swap_out([[maybe_unused]] const WeaponSwapOutArgs &args)
{

}

}}
