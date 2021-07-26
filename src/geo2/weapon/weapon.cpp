#include "geo2/weapon/weapon.h"

namespace geo2 { namespace weapon {

Weapon::Weapon(const std::shared_ptr<WeaponOwner> &owner_):
    owner(owner_)
{
    k_assert(dynamic_cast<map_obj::MapObject*>(owner_.get()) != nullptr);
}
void Weapon::start_new_level([[maybe_unused]] const WeaponStartNewLevelArgs &args)
{

}
void Weapon::swap_in([[maybe_unused]] const WeaponSwapInArgs &args)
{

}
void Weapon::swap_out([[maybe_unused]] const WeaponSwapOutArgs &args)
{

}

}}
