#pragma once

#include "geo2/weapon/weapon.h"

namespace geo2 { namespace weapon {

class TestLaser1 final: public Weapon
{
    double reload_counter;
    int ammo;
public:
    TestLaser1(const std::shared_ptr<map_obj::MapObject> &owner_);
    void run(const WeaponRunArgs &args) override;
};

}}
